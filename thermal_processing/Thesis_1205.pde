Boolean screen1;
Boolean screen5;

int fc, num = 5000;
ArrayList ballCollection; 
boolean save = false;
float scal, theta;
PGraphics letter;
PFont font;
String l = "LIVE"; 

PFont f; 
PImage bg ;
////////////neogeo
float[] randomNumbersArrayX ;
float[] randomNumbersArrayY ;
float[] speedX;
float[] speedY;
float distance = 50;
int numCircles;
boolean[] isConnected;
float distMouse;
float clickXpos;
float clickYpos;
float[] rectX;
float[] rectY;
char info[]=new char[4];
int infoint=0;
void setup() {
  background(255);
  size(1680, 1080);
 // fullScreen();
 f = createFont("Monotype  - Helvetica Now Display.otf", 16, true); 
  letter = createGraphics(width, height);
  font = createFont("Monotype  - Helvetica Now Display XBold", 20);
  ballCollection = new ArrayList();
  createStuff();
  //frameRate(24);

screen1 = false;
screen5 = false;
  /////neogeo
  numCircles = int(random(200, 300));
  randomNumbersArrayX = new float[numCircles];
  randomNumbersArrayY = new float[numCircles];
  speedX = new float[numCircles];
  speedY = new float[numCircles];
  isConnected = new boolean[numCircles];
  rectX = new float[50];
  rectY = new float[50];

  rectX[0] = 50;
  rectY[0] = 50;


  rectX[1] = 50;
  rectY[1] = 50;

  for (int i=0; i<numCircles; i++) {
    randomNumbersArrayX[i] = random(0, width);
    randomNumbersArrayY[i] = random(0, height);
    speedX[i] = random(-1, 1);
    speedY[i] = random(-1, 1);
    isConnected[i] = false;
  }
  
   
}

void draw() {
  background(255);
  ///neogeo
  //stroke(0);
  //frameRate(50);
  ///


  //screen5();
  //screen5too();
  
  if (screen1){
    screen1();
  } else if (screen5){
    screen5();
    screen5too();
  }

}

void screen1(){
  background(255, 255, 255);

  textFont(f, 80);                 
  fill(0);                         

  textAlign(CENTER, CENTER);
  text("What's happening on Twitter", width/2, height/2);
  
}
void screen5too(){
   for (int i=0; i<ballCollection.size (); i++) {
    Ball mb = (Ball) ballCollection.get(i);
    mb.run();
  }  

  //theta += .0523;
  theta += .07;

  //if (save) {
  //  if (frameCount%1==0 && frameCount < fc + 30) saveFrame("image-####.gif");
  //} 
  

}
void keyPressed() {
  if (key != CODED) {
    if(infoint<4){
   info[infoint] = key;
   String str="";
   for(int i=0;i<=infoint;i++){
    str+= info[i];
   }
   l=str;
   infoint++;
      createStuff();
    }
   
}
 if (keyCode == ENTER) {
      infoint=0;
      l="";
      createStuff();   
    }
  
if (key == CODED) {
    if (keyCode == UP) {      
      screen1 = true;
      screen5 = false;
      println("screen1 is true");
      println("key up pressed");
    } else if (keyCode == DOWN) {
      screen1 = false;
      screen5 = true;
      println("screen5 is ture");   
      }
   }
}
   



void mouseReleased() {

 bg = createImage(384*1680/1080, 384, RGB);
bg.loadPixels() ;//加载窗口像素
loadPixels();
for(int x = 0 ; x < width; x++){
for(int y = 0 ; y < height; y++){
  int x1 = int(map(x,0,width,0,bg.width));
  int y1 = int(map(y,0,height,0,bg.height));
  bg.set(x1,y1,get(x,y));
}
}

  bg.updatePixels();
  bg.save("1.png");
//updatePixels();//确认更新窗口像素颜色



  
  //createStuff();
  //fc = frameCount;
  //save = true;
//  saveFrame("image-###.gif");
 // saveFrame(bg);
}

void createStuff() {
  ballCollection.clear();

  letter.beginDraw();
  letter.noStroke();
  letter.background(255);
  letter.fill(0);
  letter.textFont(font, 600);
  letter.textAlign(LEFT);
  letter.text(l, 150, 800);
  letter.endDraw();
  letter.loadPixels();

  for (int i=0; i<num; i++) {
    int x = (int)random(width);
    int y = (int)random(height);
    //color c = letter.get(x, y);
    int c = letter.pixels[x+y*width];
    if (brightness(c)<255) {
      PVector org = new PVector(x, y);
      float radius = random(3, 8);
      PVector loc = new PVector(org.x+radius, org.y);
      float offSet = random(TWO_PI);
      int dir = 5;
      float r = random(1);
      if (r>.5) dir =-1;
      Ball myBall = new Ball(org, loc, radius, dir, offSet);
      ballCollection.add(myBall);
    }
  }
}
class Ball {

  PVector org, loc;
  float sz = 10;
  float radius, offSet, a;
  int s, dir, countC, d = 50;
  boolean[] connection = new boolean[num];

  Ball(PVector _org, PVector _loc, float _radius, int _dir, float _offSet) {
    org = _org;
    loc = _loc;
    radius = _radius;
    dir = _dir;
    offSet = _offSet;
  }

  void run() {
    display();
    move();
    lineBetween();
  }

  void move() {
    loc.x = org.x + sin(theta*dir+offSet)*radius;
    loc.y = org.y + cos(theta*dir+offSet)*radius;
  }

  void lineBetween() {
    countC = 1;
    for (int i=0; i<ballCollection.size(); i++) {
      Ball other = (Ball) ballCollection.get(i);
      float distance = loc.dist(other.loc);
      if (distance >0 && distance < d) {
        a = map(countC, 0, 10, 50, 255);
        stroke(0, a);
        strokeWeight(2);
        line(loc.x, loc.y, other.loc.x, other.loc.y);
        connection[i] = true;
      } else {
        connection[i] = false;
      }
    }
    for (int i=0; i<ballCollection.size(); i++) {
      if (connection[i]) countC++;
    }
  }

  void display() {
    noStroke();
    fill(0, 0, 0);
    ellipse(loc.x, loc.y, sz, sz);
  }
}

void mouseClicked()
{

  int mouseXpos = mouseX;
  //println("mouseclciked" + mouseX + ", " + mouseY);
  //println(clickXpos);

  for (int x = 0; x < randomNumbersArrayX.length; x++) {

    //println(x + ". " + randomNumbersArrayX[x]);
    if ((randomNumbersArrayX[x] <= mouseXpos + 50) || (randomNumbersArrayX[x] >= mouseXpos -50)) {
    //  println("ok wow that worked");
      //this is where the code to trigger printer would go
    } else {
    }
  }
}

void screen5(){
  

    for (int i=0; i < numCircles; i++) {
    isConnected[i] = false;
  }

  for (int i=0; i < numCircles; i++) {
    float distMouse = dist(mouseX, mouseY, randomNumbersArrayX[i], randomNumbersArrayY[i]);
    if (distMouse < 70) {
      //mouse connecting lines
      strokeWeight(5);
      stroke(0);  
      line(mouseX, mouseY, randomNumbersArrayX[i], randomNumbersArrayY[i]);
    }
  }

  for (int i=0; i < numCircles; i++) {
    randomNumbersArrayX[i] =  randomNumbersArrayX[i] + speedX[i];
    randomNumbersArrayY[i] =  randomNumbersArrayY[i] + speedY[i];
  }

  for (int i=0; i < numCircles; i++) {
    if ((randomNumbersArrayX[i] > width) || (randomNumbersArrayX[i] < 0)) {
      speedX[i] = speedX[i] * -1;
    }
  }

  for (int i=0; i < numCircles; i++) {
    if ((randomNumbersArrayY[i] > height) || (randomNumbersArrayY[i] < 0)) {
      speedY[i] = speedY[i] * -1;
    }
  }


  //connecting lines
  for (int i = 0; i < numCircles; i++) {
    for (int j = 0; j < numCircles; j++) {

      float dp = dist(randomNumbersArrayX[i], randomNumbersArrayY[i], randomNumbersArrayX[j], randomNumbersArrayY[j]);
      if (dp < distance) {
        if (i!=j) {
          stroke(0); 
          strokeWeight(2);
          line(randomNumbersArrayX[i], randomNumbersArrayY[i], randomNumbersArrayX[j], randomNumbersArrayY[j]);
          isConnected[i] = true;
          isConnected[j] = true;
        }
      }
    }
  }


  //stroke square
  for (int i=0; i < numCircles; i++) {

    if (isConnected[i] == true) {
      
      stroke(0);
      strokeWeight(6);
      //rectangles are drawn here
      rect(randomNumbersArrayX[i], randomNumbersArrayY[i], 5, 5); 
      clickXpos = randomNumbersArrayX[i];
      clickYpos = randomNumbersArrayY[i];
    } else {

      fill(0);
      stroke(0);
      strokeWeight(2);
      rect(randomNumbersArrayX[i], randomNumbersArrayY[i], 3, 3); 
      clickXpos = randomNumbersArrayX[i];
      clickYpos = randomNumbersArrayY[i];
    }
  }

  for (int x = 0; x < 50; x++) {

    rect(rectX[x], rectX[x], 8, 8);
  }
  
}
