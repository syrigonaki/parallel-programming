
package elevator_sim
import akka.actor.{Actor, ActorSystem, ActorRef, Props}
import scala.util.Try
import scala.util.Random
import scala.concurrent.duration._
import scala.concurrent.ExecutionContext.Implicits.global
import scala.collection.mutable
import akka.actor.Cancellable
import scala.concurrent.Await

case class Tick(minute: Int)
case class CallElevator(userId: Int, from: Int, to: Int, time: Int)
case class ElevatorAvailable(elevatorId: Int, currentFloor: Int, busy: Boolean)
case class PickupRequest(from: Int, to: Int, userId: Int)
case class EnterElevator(elevatorId: Int, minute: Int)
case class ExitElevator(floor: Int, minute: Int)
case class RegisterElevators(elevators: Vector[ActorRef])
case class Request(userId: Int, from: Int, to: Int)
case class RecordWaitingTime(time: Int)
case object PrintAverage

class Clock(totalMinutes: Int, allActors: Seq[ActorRef], manager: ActorRef) extends Actor {
  
  import context.dispatcher
  var current_minute = 0
  val scheduler = context.system.scheduler
  var cancellable: Option[Cancellable] = None
  def tick(): Unit = {
    if (current_minute < totalMinutes) {  
     
      allActors.foreach(_ ! Tick(current_minute))
      current_minute += 1
      
      cancellable = Some(context.system.scheduler.scheduleOnce(20.millisecond)(tick()))
    } else {
      println("Simulation complete.")
      cancellable.foreach(_.cancel())
      manager ! PrintAverage

      context.system.terminate()
    }
  }

  def receive: Receive = {
    case "start" =>
      tick()
    
  }

}


object elevatorSimulation {


  var elevators_n=0
  var floors_n=0
  var users_n=0
  def main(args: Array[String]): Unit =  {

   
    if (args.length != 4) {
      println("Usage: elevator simulation <number of elevators> <number of floors> <number of users> <simulation time in minutes>")
      System.exit(1)
    }
    
    elevators_n = args(0).toInt
    floors_n = args(1).toInt
    users_n = args(2).toInt
    val sim_time = args(3).toInt

    val system = ActorSystem("elevator_system")

    val manager= system.actorOf(Props(new ElevatorManager(floors_n)), name = "manager")

    val users: Map[Int, ActorRef] = (0 until users_n).map { i=> 
      i -> system.actorOf(Props(new UserActor(i, floors_n, manager)), name= s"user_$i")
    }.toMap

    val elevators: Map[Int, ActorRef] = (0 until elevators_n).map { i=> 
      i -> system.actorOf(Props(new ElevatorActor(i, floors_n, manager, users)), name=s"elevator_$i")
    }.toMap

    
    manager ! RegisterElevators(elevators.values.toVector)

    val allActors = users.values.toSeq ++ elevators.values.toSeq
    val clock = system.actorOf(Props(new Clock(sim_time, allActors, manager)), name="clock")
    
  
    clock ! "start"
    
    Await.result(system.whenTerminated, Duration.Inf)
  }

}





class UserActor(userId: Int, floors_n: Int, manager: ActorRef) extends Actor {
  var current_floor = 0
  var destination_floor = -1
  var waiting_duration = Random.between(1,61) 
  var waiting_time=0 //waiting start time
  var elev_called = false
  var in_elevator = false
  var call_wait_time=0

  def receive = {
    case Tick(minute) =>
     
      if (elev_called == false && (minute-waiting_time) >= waiting_duration) {
        
        do {//pick a random floor different except the current one
          destination_floor = scala.util.Random.nextInt(floors_n)
        } while (destination_floor == current_floor)
        
        manager ! CallElevator(userId, current_floor, destination_floor, minute)
        call_wait_time = 0-minute //to calculate wait time average
        println(s"Minute $minute: User $userId calls elevator at floor $current_floor")
        elev_called = true
      }

    case EnterElevator(elevatorId, minute) =>  
      in_elevator = true
      call_wait_time += minute  //calculate and record waiting time
      manager ! RecordWaitingTime(call_wait_time) 
      println(s"Minute $minute: User $userId enters elevator $elevatorId at floor $current_floor with destination floor $destination_floor")

    case ExitElevator(floor, minute) =>
      current_floor = floor
      waiting_time = minute
      in_elevator = false
      println(s"Minute $minute: User $userId exits elevator at floor $floor")

      elev_called = false
      destination_floor = -1
      waiting_duration = Random.between(1,61) //start waiting to call elevator again
      waiting_time = minute
    }

} 



class ElevatorActor(elevatorId: Int, floors_n: Int, manager: ActorRef, users: Map[Int, ActorRef]) extends Actor {

  var current_floor = Random.between(1,floors_n)
  var direction = 1 // 1 = up, -1 = down
  var destinations = Set.empty[Int]
  var users_onboard = Map.empty[Int,Int]
  var requests = List.empty[Request]
  var stopped = true //stopped at floor
  var busy = false //true when loading/unloading passengers

  override def preStart(): Unit = { //initialize elevators at a random floor
    println(s"Minute 0: Elevator $elevatorId starting at floor $current_floor")
    manager ! ElevatorAvailable(elevatorId, current_floor, busy) //notify elevatorManager about the available elevators
  }


  def hasDestinationsInDirection(): Boolean = { //checks if there is any request to answer in the current direction
    if (direction == 1) destinations.exists(_ > current_floor)
    else destinations.exists(_ < current_floor)
  }

  def receive: Receive = {
    case Tick(minute) =>
      
      if (stopped==false && busy == false) {
        
        if (destinations.nonEmpty) {
          stopped = false
          val next_floor = current_floor + direction
          if (next_floor<0 || next_floor >= floors_n || hasDestinationsInDirection()==false) { //change direction if there are no requests that way or when at the top/bottom
            direction *= -1
            current_floor += direction
          } else {
            current_floor = next_floor
          }
          println(s"Minute $minute: Elevator $elevatorId reaches floor $current_floor")
          if (destinations.contains(current_floor)) {
            stopped = true
            busy = true
          }

        manager ! ElevatorAvailable(elevatorId, current_floor, busy) //notify elevatorManager about the changes (used to assign elevators to requests)

        } else {
          println(s"Empty elevator $elevatorId stopped at floor $current_floor")
          stopped = true
          busy = false
        }

      } else if (stopped==true && busy == true) { //load/unload passengers

        val exiting_users = users_onboard.filter(_._2 == current_floor).keys
          stopped = true
          if (exiting_users.nonEmpty) {
            println(s"Minute $minute: Elevator $elevatorId stops at floor $current_floor")
            exiting_users.foreach { userId =>
              users(userId) ! ExitElevator(current_floor, minute)
          }

          users_onboard --= exiting_users
          destinations -= current_floor
      
        }

        val users_to_enter = requests.filter(_.from == current_floor)
          stopped = true
          users_to_enter.foreach { req =>
            users(req.userId) ! EnterElevator(elevatorId, minute)
            users_onboard += (req.userId -> req.to)
            destinations += req.to
          }

          requests = requests.filterNot(_.from == current_floor)  //update requests & destinations
          if (users_to_enter.nonEmpty) {
            destinations-=current_floor
          }
                
          busy = false
          stopped = false
        }
    

      case Request(userId, from, to) => 
        requests :+= Request(userId, from, to)
        destinations += from

        if (busy == false && stopped == true) { //if elevator was stopped with no requests start moving again
          if (from<current_floor) direction = -1
          else if (from>current_floor) direction = 1
          stopped = false
        }
  }
  
}

class ElevatorManager(floors_n: Int) extends Actor {
  var elevators = Vector.empty[ActorRef]
  var elevator_states = Map[Int, (Int, Boolean)]()
  var waiting_times = List.empty[Int]

  def receive: Receive = {
    case CallElevator(userId, from, to, time) =>
      val best_elevator = chooseElevator(from)
      
      if (best_elevator.isDefined) {
        val elev = elevators(best_elevator.get)
        elev ! Request(userId, from , to)
      } 
    
    case ElevatorAvailable(elevatorId, current_floor, busy) =>
      elevator_states = elevator_states + (elevatorId -> (current_floor, busy))

    case RegisterElevators(elevs) =>
      elevators = elevs.toVector
    case RecordWaitingTime(time) =>
      waiting_times ::= time
    case PrintAverage =>
      val average = if (waiting_times.nonEmpty) waiting_times.sum.toDouble / waiting_times.size else 0.0
      println(s"The average waiting time is $average minutes")
  
  }

  def chooseElevator(user_floor: Int): Option[Int] = {  //choose elevator by getting the closest non-busy one or closest busy one if all elevators are busy
    val nonBusyElevators = elevator_states.filter(_._2._2 == false).keys.toSeq

    val candidates =
      if (nonBusyElevators.nonEmpty) nonBusyElevators
      else elevator_states.keys.toSeq

    if (candidates.isEmpty) None
    else Some(candidates.minBy(elevatorId => math.abs(elevator_states(elevatorId)._1 - user_floor)))
  }

}

   
