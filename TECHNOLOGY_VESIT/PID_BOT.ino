/*--------------IR SENSOR ARRAY----------------*/
#define IR5 2
#define IR4 3
#define IR3 4
#define IR2 5
#define IR1 6

/* ---------- PID ---------- */
float Kp = 1.0f;
float Ki = 0.0f;
float Kd = 0.0f;

float prevError = 0, integral = 0;

/* ---------- MOTORS ---------- */
float baseSpeed = 45.0;   // (period = 255)
float leftSpeed, rightSpeed;

/*-----IR VALUES-----------*/
uint8_t ir[5];            

void setup() {
  // put your setup code here, to run once:
  pinMode(IR1, INPUT);
  pinMode(IR2, INPUT);
  pinMode(IR3, INPUT);
  pinMode(IR4, INPUT);
  pinMode(IR5, INPUT);
  pinMode(9, OUTPUT);
  pinMode(12, OUTPUT);
  Serial.begin(9600);
}

void loop() {
  // put your main code here, to run repeatedly:
  IR_Sensor();
  float error = CalculateError();

  int sharpTurn = 0;

  /* Extreme sensor active → sharp corner */
  if (fabs(error) > 1.5f && fabs(prevError) < 0.5f) {
    sharpTurn = 1;
  }

  float dynamicBase;
  float pid = PID_Controller(error);
  // pid = clamp(pid, -60, 60);
  Serial.println(pid);

  // if (sharpTurn) {
  //   /*  FORCE TURN MODE */
  //   dynamicBase = 50;           // very slow
  //   pid = clamp(pid, -80, 80);  // stronger correction
  // }

  // else {
  //   float absError = fabs(error);
  //   float absDeriv = fabs(error - prevError);
  //   dynamicBase = baseSpeed - (absError * 15) - (absDeriv * 20);
  //   dynamicBase = clamp(dynamicBase, 45, baseSpeed);
  // }

  leftSpeed = dynamicBase - pid;
  rightSpeed = dynamicBase + pid;
  // Serial.print(leftSpeed);
  // Serial.print(" ");
  // Serial.println(rightSpeed);
  SetMotor(leftSpeed , rightSpeed);
  delay(10);
}

/*----------------FUNCTIONS-------------*/
float CalculateError(void)
{
    float w[5] = {40.0,25.0,0.0,-25.0,-40.0};
    float sum=0, err=0;

    for(int i=0;i<5;i++)
    {
        sum += ir[i];
        err += w[i]*ir[i];
    }

    if (sum == 0)
    {
        integral = 0;                 // VERY IMPORTANT
        prevError = clamp(prevError, -2.0f, 2.0f);
        return prevError;
    }

    return err/sum;
}

float PID_Controller(float error)
{
    integral += error;
    integral = clamp(integral,-60,60);

    static float dFiltered = 0;
    float dRaw = error - prevError;
    dFiltered = 0.6f * dFiltered + 0.4f * dRaw;
    prevError = error;

    return ((Kp*error) + (Ki*integral) + (Kd*dFiltered));
}


/*----------------SENSOR CONFIGURATION-----------------*/
void IR_Sensor() {
  ir[0] = !digitalRead(IR1);
  ir[1] = !digitalRead(IR2);
  ir[2] = !digitalRead(IR3);
  ir[3] = !digitalRead(IR4);
  ir[4] = !digitalRead(IR5);
// for(int i =0;i<5;i++){
//   Serial.print(ir[i]);
//   Serial.print(" ");
// }
// Serial.println();
}


/*---------------MOTOR CONFIGURATION-----------*/
void SetMotor(int left, int right) {
  left = clamp(left, 0, 100);
  right = clamp(right, 0, 100);
  analogWrite(10, right);
  analogWrite(11, left);
  digitalWrite(9, 0);
  digitalWrite(12, 0);
}

/*-------------CLAMP CONDITION--------*/
float clamp(float value, float minVal, float maxVal)
{
  if (value < minVal) return minVal;
  if (value > maxVal) return maxVal;
  return value;
}
