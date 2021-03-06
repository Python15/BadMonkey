  // code qui parcours la pièce
// How to use it :
// - open the Arduino serial console

#include <Arduino.h>
#include <Console.h>

#include <Bridge.h>
#include <HttpClient.h>
#include <YunServer.h>
#include <YunClient.h>

#include <Wire.h>

#include <Adafruit_MotorShield.h>
#include "utility/Adafruit_PWMServoDriver.h"

#define DEFAULT_STEP_NUMBER		100
#define INFRARED_SENSOR_INPUT	A0

#define SCAN_ANGLE 	60

//Robot parameters
#define WHEEL_DIAM  69 // mm
#define WHEEL_PERIM  218 // mm
#define STEPMOTOR  200  // pas du moteur pour faire 360°
#define RADIAN_PER_DEG  3.14159265359/180 
#define TRACK  123  // Entraxe entre le mileu des roues
#define ENCODER_FACTOR  WHEEL_PERIM/20 
#define COEFF_CORRECTION 1 //Coefficient correcteur pour le frottement

Adafruit_MotorShield AFMS = Adafruit_MotorShield(); 
Adafruit_StepperMotor * motor1 = AFMS.getStepper(200, 2);
Adafruit_StepperMotor * motor2 = AFMS.getStepper(200, 1);

int counter;

//Parametres pour l'odometrie
unsigned int  traveled_distance;  //Distance totale parcourue
float theta = 0;      // Orientation du robot
float posRob[2];      // Coordonées du robot
float posObj[2];  // Coordoonées d'un obstacle

int etatHttp;
int etatParcours=2;

YunServer server;
bool run = false;

void motorInitialize()
{	
	AFMS.begin();
	
	// rotation per min
	motor1->setSpeed(100); 
  	motor2->setSpeed(100);  
  	motor1->release();
  	motor2->release(); 
}


int recordIR()
{
  int cumulAnalogValue = 0;
  int analogValue;
  int samples = 5;
  for (int i = 1; i <= samples; i++)
  {
    analogValue = analogRead(INFRARED_SENSOR_INPUT);
    cumulAnalogValue += analogValue; 
    //Console.println("Instant value :");
    //Console.println(analogValue);
    delay(5);   
  }
  //Console.println("Mean value :");
  //Console.println(cumulAnalogValue/samples);
  return((int)cumulAnalogValue/samples);
   
}

float getDistance()
{
  int valueSensor= recordIR();
  double distanceCM;
  distanceCM = 
  pow(valueSensor, 6)*(2.76075/100000000000000) +
  pow(valueSensor, 5)*(-6.99111/100000000000) +
  pow(valueSensor, 4)*(7.15462/100000000) +
  pow(valueSensor, 3)*(-3.80057/100000) +
  pow(valueSensor, 2)*0.011194502 +
  (valueSensor*(-1.807762943)) +
  146.981348;
  return ((float)distanceCM);
}

//Marche avant
void motorForward()
{
	for (int i=0; i < 275; i++)
	{
		motor1->step(1, FORWARD, SINGLE);
		motor2->step(1, FORWARD, SINGLE); 
	}
        calculPosition (DEFAULT_STEP_NUMBER,DEFAULT_STEP_NUMBER);
}

//Marche avant de X mm
void motorForward(int lenght_to_do)
{
    int steptodo = (double)lenght_to_do / (double)WHEEL_PERIM * (double) 200 + 1;
    
	Console.println("motorForward");
        Console.println(steptodo);
        
        motor2->step(1, FORWARD, SINGLE);
        for (int i=0; i < (steptodo-2) ; i++)
	{
		motor1->step(1, FORWARD, SINGLE);
		motor2->step(1, FORWARD, SINGLE); 
	}
	motor1->step(1, FORWARD, SINGLE);
        //Maj de la position
        calculPosition (steptodo,steptodo);
}


//Marche arriere de X mm
void motorBackward(int lenght_to_do)
{
        int steptodo = (double)lenght_to_do / (double)WHEEL_PERIM * (double) 200 + 1;
	for (int i=0; i < steptodo; i++)
	{
		motor1->step(1, BACKWARD, SINGLE); 
		motor2->step(1, BACKWARD, SINGLE); 
	}
        //Maj de la position
        calculPosition (-steptodo,-steptodo);
}

//Marche arriere par DEFAUT
void motorBackward()
{       
	for (int i=0; i < DEFAULT_STEP_NUMBER; i++)
	{
		motor1->step(1, BACKWARD, SINGLE); 
		motor2->step(1, BACKWARD, SINGLE); 
	}
        //Maj de la position
        calculPosition (-DEFAULT_STEP_NUMBER,-DEFAULT_STEP_NUMBER);
}


//Calcul la position du robot (Odometrie)
void calculPosition (int step_left, int step_right)
{        
        // 1 Calcul de la distance parcouru par chaque roue 
        float distance_wheel_left  =  (float) step_left * (float)WHEEL_PERIM / (float)STEPMOTOR;
        float distance_wheel_right = (float) step_right * (float)WHEEL_PERIM /  (float)STEPMOTOR;
        float distance = (distance_wheel_left + distance_wheel_left)/2;
        
        //Maj de la distance totale parcourue
        traveled_distance= traveled_distance + abs(distance)/10;

        //2 Conversion de l'angle theta en radian
        float thetaRadian = theta * PI / 180;
        
        //3 Calcul de la position
        float yPrime = distance * cos(thetaRadian);
        float xPrime = distance * sin(thetaRadian);
        
        //4 Incrementation avec les positions précedentes
        posRob[0] = posRob[0] + xPrime;
        posRob[1] = posRob[1] + yPrime;
        
        
}



//Convertie des degres en step moteur
int calcul_step_turn(int degree_to_turn){
    
    float x = ((float)degree_to_turn) /((float) 360);
    float distance_for_turn = x * PI * (float) TRACK;
    int step_to_turn = 1 + distance_for_turn * STEPMOTOR / WHEEL_PERIM;

    return step_to_turn;
}

//Fonction tourne a gauche de X degres
void turnDegreeLeft(int degree_to_turn){
  
    int steptodo = calcul_step_turn(degree_to_turn);
	for (int i=0; i < steptodo; i++)
	{
		motor1->step(1, FORWARD, SINGLE); 
		motor2->step(1, BACKWARD, SINGLE); 
	}
        
        //Maj de l'orientation du robot
        theta = ((int)theta + degree_to_turn) % 360;
}

//Fonction tourne a droite de X degres
void turnDegreeRight(int degree_to_turn){
  
    int steptodo = calcul_step_turn(degree_to_turn);
    for (int i=0; i < steptodo; i++)
	{
		motor1->step(1, BACKWARD, SINGLE); 
		motor2->step(1, FORWARD, SINGLE); 
	}
        
        //Maj de l'orientation du robot
        theta = ((int)theta - degree_to_turn) % 360;

}


//Fonction qui calcul la position d'un point d'un obstacle
void localizePointObject (float irDetection){
  
     irDetection += 45;
     //1 Conversion de l'angle theta en radian
     float thetaRadian = theta * PI / 180;
     
     //2 Calcul de X, Y du point detecté par rapport au robot
     float posObjX = irDetection * sin (thetaRadian);
     float posObjY = irDetection * cos (thetaRadian);
     
     //3 Translation des coordonées par rapport au repère base
     //Enregistrement dans le tableau
     posObj [0] =  posRob[0] + posObjX;
     posObj [1] =  posRob[1] + posObjY;
    
}


void updateServer()
{
  HttpClient client;
  Console.println("Envoie requête HTTP au serveur");
  //String url = "http://perso.imerir.com:80/jloeve/savePoint.php";
  String url = "http://172.31.1.123:80/savePoint.php?xr="+String((int)posRob[0], DEC)+"&yr="+String((int)posRob[1], DEC)+"&xm="+String((int)posObj[0], DEC)+"&ym="+String((int)posObj[1], DEC)+"&a="+String((int)theta, DEC);  
  Console.println(url);  
  client.get(url);
  while (client.available()){
    char c = client.read();
    Console.print(c);
  }
}
 
 
float distanceMin=90;
float lastDistance=0;
short int angleDistMin=0;

int distAvance;
void stateMachine()
{

    switch (etatParcours)
    {
      case 1:
      Console.println("cas 1, avance ou scan");  
        
            Console.println(distAvance);
            motorForward(distAvance);
            etatParcours=2;
          
        break;
        
     case 2:
      Console.println("cas 2, Mur ou pas");
        lastDistance= getDistance() * 10;
          Console.println(lastDistance);          
            if(lastDistance < 130)
            {  
              
                //tourner à droite
                Console.println("oui");
                etatParcours=3;
              
            }
            else if(lastDistance < 250)
              {   
                etatParcours=4;
              }else{
              Console.println("non");
              distAvance = lastDistance - 250;
               etatParcours=1;
            }
         
          break;
      case 3:
      Console.println("cas 3, Tourne Random");
        localizePointObject(lastDistance);
        updateServer();
        motorBackward(50);
        turnDegreeRight(random(45,180));
        etatParcours=4;
        break;
          
      case 4:
        Console.println("cas 4, balayage");
        //verifier si l'on a bien le point le plus proche a +/-10°
        turnDegreeRight(SCAN_ANGLE/2);
        etatParcours=1;
          lastDistance= getDistance() * 10;
          if ( lastDistance < 130)
          {
            etatParcours=3;
          }
          turnDegreeLeft(SCAN_ANGLE);
          lastDistance= getDistance() * 10;
          if ( lastDistance < 130)
          {
            etatParcours=3;
          }
        turnDegreeRight(SCAN_ANGLE/2);
        distAvance = lastDistance - 100;        
        break; 
    }
}

void radar()
{
  for (int cpt = 0; cpt < 360; cpt ++)
 {
     lastDistance= getDistance() * 10;     
      Console.println(lastDistance);
     localizePointObject(lastDistance);
     updateServer();
     turnDegreeRight(1);
 }  
}

void singleScan()
{
     lastDistance= getDistance() * 10;     
     Console.println(lastDistance);
     localizePointObject(lastDistance); 
     updateServer(); 
}

void resetOdo()
{
    theta = 0;      // Orientation du robot
    posRob[0];      // Coordonées du robot
    posRob[1];      // Coordonées du robot
    posObj[0];  // Coordoonées d'un obstacle
    posObj[1];  // Coordoonées d'un obstacle
}

void executeUrlCommand(YunClient client)
{
	String command = client.readStringUntil('/');
        
	Console.print("Execute ");
	Console.print(command);
	Console.print(" arg: ");
        client.println("Status: 200");
        client.println("Access-Control-Allow-Origin: *");

	if (command=="start")
	{
		run = true;
	}
	else if (command=="stop")
	{
		run = false;
	}
	else if (command=="fwd")
	{
		Console.println(client.readString());
                Console.println("Move forward...");
                motorForward(10);
	}
	else if (command=="bwd")
	{
		Console.println(client.readString());
                Console.println("Move backward...");
                motorBackward(10);
	}
	else if (command=="left")
	{
		Console.println(client.readString());
                Console.println("Move left...");
                turnDegreeLeft(10);
	}
	else if (command=="right")
	{
		Console.println(client.readString());
                Console.println("Move right...");
                turnDegreeRight(10);
	}
	else if (command=="scan")
	{
		Console.println(client.readString());
                Console.println("Scanning one point");
                singleScan();
	}
	else if (command=="radar")
	{
		Console.println(client.readString());
                Console.println("Scanning radar mode...");
                radar();
	}
	else if (command=="reset")
	{
		Console.println(client.readString());
                Console.println("Reseting odomtry...");
                resetOdo();
	}
        else
	{
		Console.print("Huuu?? ");
		Console.println(client.readString());
	}
}

void setup()
{
	Bridge.begin();
  	Console.begin();

  	server.listenOnLocalhost();
  	server.begin();

	while (!Console)
  	{
  		// wait Arduino Console connection.
  	}

	//Console.println("Simple loop for infrared sensor !");

	pinMode(INFRARED_SENSOR_INPUT, INPUT);
        motorInitialize();
}



void loop()
{	
  YunClient client = server.accept();
  if (client)
  {
     executeUrlCommand(client);
     client.stop();
  }
  if (run)  	
    stateMachine();
  else
    delay(100);
}

