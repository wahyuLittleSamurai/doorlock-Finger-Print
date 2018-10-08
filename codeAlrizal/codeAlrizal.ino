/*
 * ###################################################|
 * Autor: Wahyu J                                     |
 * Phone: 085850474633                                |
 * Web: yhooragrup.com || konsultansoftware.com       |
 * IG: yhooragrup || konsultansoftware || shorkutkey  |
 * ###################################################|
 */

#include  <Wire.h>  //add library
#include  <LiquidCrystal_I2C.h> //add library i2c untuk lcd
#include <Keypad.h>       //library keypad
#include <Adafruit_Fingerprint.h> //library fingerprint
#include <SoftwareSerial.h> //library untuk komunikasi serial lebih dari 1
#include <EEPROM.h> //eeprom lib

SoftwareSerial mySerial(6, 5);  //pin TX dan RX untuk Finger print
SoftwareSerial mySim(3, 4); //pin TX dan RX untuk sim

#define on  LOW               
#define off HIGH
#define magnet A7           //definisi pin magnet di A7
#define buzzer  A0          
#define valve A3
#define resetPin  2
#define maxId 5             //definisi max id untuk finger print

#define redLed  1           //pin red led
#define yelLed  0           //pin yellow led

const byte ROWS = 5; //four rows
const byte COLS = 4; //four columns
char hexaKeys[ROWS][COLS] = {
  {'&','@','#','*'},
  {'1','2','3','^'},
  {'4','5','6','v'},
  {'7','8','9','|'},
  {'<','0','>','~'},
};

LiquidCrystal_I2C lcd(0x3F, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE);  // Set the LCD I2C address
char lcdBuff[16];       //variable untuk menyimpan data supaya bisa di tampilkan ke lcd lebih rapi

byte rowPins[ROWS] = {13, 12, 11, 10, 9}; //connect to the row pinouts of the keypad
byte colPins[COLS] = {A2, A1, 7, 8}; //connect to the column pinouts of the keypad

//initialize an instance of class NewKeypad
Keypad customKeypad = Keypad( makeKeymap(hexaKeys), rowPins, colPins, ROWS, COLS); 

Adafruit_Fingerprint finger = Adafruit_Fingerprint(&mySerial);  //object untuk finger print

uint8_t id;                   //variable untuk menyimpan data ID finger print
String tampungKeypad = "";  //menampung variable keypad yg di ubah ke dalam bentuk string 
String passwordAdmin;       //menampung variable String untuk password
int passwordAdminArray[5];    //menampung per character setiap masing2 character password
int numberAdminArray[15];   //menampung per character setiap digit nomer supaya bisa di simpan ke eeprom per digit nomer 
int counterKeypad = 0;      //menampung berapa kali tekan keypad
uint16_t currentMillis = 0;   //menampung nilai waktu millis
uint16_t lastMillis = 0;      //nilai terakhir millis
uint16_t setPointTime = 200;  //jeda waktu ketika magnet sensor tidak terbaca

boolean menuCRUD = false;     //variable - variable yg digunakan untuk penanda program utama
boolean daftar = false;
boolean deleteUser = false;
boolean verify = false;
boolean verifyFinger = false;
boolean absens = false;

//## variable yg digunakan di program
int maxTime,counterCommand, wrongPassword, wrongFinger;
boolean found = false;
boolean autoReset = false;
boolean newSms = false;
boolean doorOpen = false;
boolean changePassword = false;
boolean changeNumber = false;
boolean adminLogin = false;

void setup() 
{
  finger.begin(57600);      //inisialisasi kecepatan transfer data antara arduino dengan finger print
  mySim.begin(9600);        //inisialisasi kecepatan transfer data antara arduino dengan sim
 
  lcd.begin(16,2);   // initialize the lcd for 16 chars 2 lines, turn on backlight
  pinMode(magnet, INPUT_PULLUP);  //magnet di pull up yg artinya akan membaca 1 ketika tidak ada inputan
  pinMode(buzzer, OUTPUT);  //buzzer output
  pinMode(valve, OUTPUT);
  pinMode(redLed, OUTPUT);
  pinMode(yelLed, OUTPUT);

  digitalWrite(redLed, HIGH);
  digitalWrite(yelLed, LOW);

  digitalWrite(buzzer, on);
  delay(1000);
  digitalWrite(buzzer, off);

  mySerial.listen();              //switch ke mode finger untuk pembacaan Serial, harus gantian dengan sim 
  if (finger.verifyPassword()) {  //cek komunikasi dengan finger
    lcd.setCursor(0,0);
    lcd.print("Finger Found");
    delay(1000);
    lcd.clear();
  } else {
    lcd.setCursor(0,0);
    lcd.print("Finger Not Found");
    while (1) { delay(1); }
  }
  finger.getTemplateCount();
  
  lcd.setCursor(0,0);
  lcd.print("Please Wait...");
  mySim.listen();
  delay(15000);         //jeda waktu sampai sim mendapatkan signal, kedil 1x dalam 2 detik, jika cuma 1x berarti belum dapat signal
  
  //## program untuk setting sim800 sebagai mode text dan menghapus seluruh SMS yg ada, agar tidak error
  while(counterCommand<=3)  //looping sampai pengaturan sim selesai
  {
    switch(counterCommand)
    {
      case 0: atCommand("AT",1,"OK");break; //cek komunikasi sim dengan arduino
      case 1: atCommand("AT+CMGF=1",1,"OK");break;  //setting ke mode text, dengan menunggu balasan selama 1 detik dengan balasan OK
      case 2: atCommand("AT+CMGL=\"ALL\",0",2,"OK");break;  //baca seluruh SMS yg masuk, optional
      case 3: atCommand("AT+CMGD=1,4",1,"OK");break;  //Delete seluruh sms yg masuk, agar tidak terjadi hang ketika pembacaan sms, optional jika ada feature akses dengan sms
    }
  }

  counterCommand = 0;
  
  lcd.clear();
  verify = true;
  tampungKeypad = "";
  adminLogin = true;
  while(verify)         //perintah memasukkan password ketika awal system nyala
  {
    verifyPassword2();    //panggil func veryPassword2();
    menuCRUD = false; //penanda menuCRUD = false
  }
  
}

void loop() 
{
  mySerial.listen();  //switching ke mode serial untuk finger

  while(verifyFinger) //verifikasi finger untuk pertama kali system dinyalakan
  {
    lcd.setCursor(0,0);
    lcd.print("Tap Your Finger");
    getFingerprintIDez(); //panggil func getFingerprintIDez();
  }
  
  digitalWrite(buzzer, off);  
  currentMillis = millis();
  lcd.setCursor(0,0);             //tampilkan berikut ketika awal menu
  lcd.print("Smart Door Lock");
  lcd.setCursor(0,1);
  lcd.print("F1:Menu | F2:Abs");
  
  while(absens) //jika di pilih f2/ absens
  {
    getFingerprintIDez(); //panggil getFingerprintIDez
  }
  
  if(analogRead(magnet) <= 10 ) //jika magnet membaca inputan (pintu ketutup)
  {
    lastMillis = currentMillis; //simpan waktu sekarang ke lastMillis
    digitalWrite(buzzer, off);  
    doorOpen = false;
  }
  else  //jika pintu terbuka
  {
    if((currentMillis - lastMillis) > setPointTime) //waktu yg di simpan ketika tertutup di bandingkan dengan lama waktu terbuka, jika lebih besar maka...
    {
      digitalWrite(buzzer, on); //nyalakan buzzer
      lcd.setCursor(0,1);
      lcd.print("Close The Door!!");  //tampilkan
      delay(500);

      if(!doorOpen) //penanda untuk kirim SMS sekali ketika pintu terbuka tanpa password/finger
      {
        mySim.listen();       //switching ke mode serial sim
        atCommand("AT+CMGF=1",1,"OK");  //set mode text untuk sim900

        String numberToSend="";
        for(int epromNumber=10; epromNumber<22; epromNumber++)  //ambil nilai nomer tujuan
        {
          numberToSend += String(EEPROM.read(epromNumber));
        }
        
        atCommand("AT+CMGS=\"" +numberToSend+ "\"",1,">");//setting nomer tujuan  
        atCommand("Door Is Open",1,">");//isi dari sms
        Serial.println("Mengirim Char Ctrl+Z / ESC untuk keluar dari menu SMS");
        mySim.println((char)26);//ctrl+z untuk keluar dari menginputkan isi sms
        delay(2000); //tahan selama 1 detik
    
        mySerial.listen();  //switching ke mode serial finger
        
        doorOpen = true;  //penanda sekali untuk sms.
      }
    }
  }
  
  char customKey = customKeypad.getKey();
  
  if (customKey){
    if(customKey == '&')      //jika di tekan &(F1)
    {
      counterKeypad = 0;
      tampungKeypad = "";
      lcd.clear();
      verify = true;        //penanda untuk memasukan password dan finger ke menu admin
    }
    if(customKey == '@')  //jika di tekan @(F2)
    {
      absens = true;  //maka masuk mode absens
    }
  }
  while(verify) //
  {
    verifyPassword2();  //ferifikasi password dan finger
  }
  while(menuCRUD) //menu CRUD untuk admin
  {
    lcd.setCursor(0,0);
    lcd.print("F2:Reg v:P #:Del");
    lcd.setCursor(0,1);
    lcd.print("*:Del All ^:Pass");
    customKey = customKeypad.getKey();
    if(customKey)
    {
      if(customKey == '@')  //menu F2 untuk register user baru
      {
        daftar = true;
        lcd.clear();
        tampungKeypad = " ";
        while(daftar)
        {
          daftarFinger(); //panggil func daftarFinger
        }
      }
      if(customKey == '#')  //untuk delete ID
      {
        lcd.clear();
        deleteUser = true;
        counterKeypad = 0;
        tampungKeypad = " ";
        while(deleteUser)
        {
          lcd.setCursor(0,0);
          lcd.print("Masukan No ID: ");
          char customKey = customKeypad.getKey();
          if(customKey){
            if(customKey == '|')
            {
              tampungKeypad = " ";
              lcd.clear();
              daftar = false;
            }
            if(customKey != '~' && customKey != '&' && customKey != '@' &&
               customKey != '#' && customKey != '*' && customKey != '^' &&
               customKey != '>' && customKey != '<' && customKey != 'v'
            )
            {
              lcd.setCursor(counterKeypad,1);
              lcd.print(customKey);
              counterKeypad++;
              tampungKeypad += String(customKey);
            }
            
            else
            {
              id = tampungKeypad.toInt();
              lcd.clear();
              deleteFingerprint(id);
              deleteUser = false;
            }
          }
        }
          
      }
      if(customKey == '*')      //untuk delete seluruh finger
      {
        for(int i=0; i<=127; i++)
        {
          deleteFingerprint(i);
        }
      }
      if(customKey == '|')  //back menu utama
      {
        lcd.clear();
        menuCRUD = false;
      }
      if(customKey == '^')  //masuk ke menu ganti password
      {
        lcd.clear();
        counterKeypad = 0;
        tampungKeypad = "";
        changePassword = true;
        menuCRUD = false;
      }
      if(customKey == 'v')  //masuk ke menu untuk ganti nomer tujuan
      {
        lcd.clear();
        counterKeypad = 0;
        tampungKeypad = "";
        changeNumber = true;  //penanda nomer tujuan 
        menuCRUD = false;
      }
    }
  }
  while(changeNumber) //program untuk mengganti nomer tujuan
  {
    lcd.setCursor(0,0);
    lcd.print("New Number:");
    customKey = customKeypad.getKey();
    if(customKey)
    {
      if(customKey != '~' && customKey != '&' && customKey != '@' &&
         customKey != '#' && customKey != '*' && customKey != '^' &&
         customKey != '>' && customKey != '<' && customKey != 'v'
      )
      {
        lcd.setCursor(counterKeypad,1); //set posisi penampilan ke LCD
        numberAdminArray[counterKeypad] = atoi(&customKey); //konversi data dari character ke integer supaya bisa di simpan ke eeprom
        lcd.print(numberAdminArray[counterKeypad]); //tampilkan ke lcd
        if(counterKeypad < 13)      //jika nomer yg di inputankan < 13 hitung berapa banya digit
        { 
          counterKeypad++;  //banyak digit
        }
      }
      else        //jika tekan enter
      {
        for(int i=0; i<counterKeypad; i++)
        {
          EEPROM.write(i+10, numberAdminArray[i]);  //simpan digit nomer ke alamat eeprom di mulai dari 10 sampai banyaknya nilai variable counterKeypad
        }
       
        lcd.clear();
        lcd.setCursor(0,0);
        lcd.print("Success!!!");
        delay(2000);
        lcd.clear();
        counterKeypad = 0;
        tampungKeypad="";
        changeNumber = false;
      }
    }
  }

  while(changePassword) //ganti password, program ini hampir sama dengan ganti nomer, beda di alamat eeprom saja
  {
    lcd.setCursor(0,0);
    lcd.print("New Password:");
    customKey = customKeypad.getKey();
    if(customKey)
    {
      if(customKey != '~' && customKey != '&' && customKey != '@' &&
         customKey != '#' && customKey != '*' && customKey != '^' &&
         customKey != '>' && customKey != '<' && customKey != 'v'
      )
      {
        lcd.setCursor(counterKeypad,1);
        passwordAdminArray[counterKeypad] = atoi(&customKey);
        lcd.print(passwordAdminArray[counterKeypad]);
        if(counterKeypad < 3)
        {
          counterKeypad++;
        }
      }
      else
      {
        for(int i=0; i<=counterKeypad; i++)
        {
          EEPROM.write(i, passwordAdminArray[i]);   //alamat eeprom dari 0 - variable counterKeypad
          
        }
        lcd.clear();
        lcd.setCursor(0,0);
        lcd.print("Success!!!");
        delay(2000);
        lcd.clear();
        counterKeypad = 0;
        tampungKeypad="";
        changePassword = false;
      }
    }
  }
  
  //daftarFinger();
}
void verifyPassword2()    //func untuk verifyPassword2, yg akan di panggil di atas
{
  lcd.setCursor(0,0);
  lcd.print("Password: ");
  char customKey = customKeypad.getKey();
  if(customKey){
    if(customKey == '|')
    {
      tampungKeypad = "";
      lcd.clear();
      verify = false;
    }
    if(customKey != '~' && customKey != '&' && customKey != '@' &&
       customKey != '#' && customKey != '*' && customKey != '^' &&
       customKey != '>' && customKey != '<' && customKey != 'v'
    )
    {
      lcd.setCursor(counterKeypad,1);
      lcd.print(customKey);
      counterKeypad++;
      tampungKeypad += String(customKey);
    }
    
    else  
    {
      String newPassword = tampungKeypad;
      counterKeypad = 0;
      passwordAdmin = "";
      wrongPassword++;
      for(int countPass=0; countPass<=3; countPass++)
      {
        passwordAdmin += String(EEPROM.read(countPass));
      }
      if(newPassword == passwordAdmin)      //cek apakah inputan yg dimasukan nilainya sama dengan data eeprom yg telah di setting
      {
        verify = false;
        verifyFinger = true;
      }
      else  //jika tidak sama maka...
      {
        if(wrongPassword > 4) //apakah sudah pernah salah sebanyak > 4x
        {
          lastMillis = millis();
          long int lastMillisSecond = millis();
          lcd.clear();
          while((millis() - lastMillis) < (10*60000)) //tampilkan selama 10 menit
          {
            if((59 - ((millis()-lastMillisSecond)/1000)) == 0)
            {
              lastMillisSecond = millis();
            }
            
            lcd.setCursor(0,0);
            lcd.print("Please Wait...");
            lcd.setCursor(0,1);
            lcd.print(9 - ((millis()-lastMillis)/60000)); 
            lcd.print(":");
            long int secondS = 59 - ((millis()-lastMillisSecond)/1000);
            lcd.setCursor(2,1);
            sprintf(lcdBuff,"%2d", secondS);
            lcd.print(lcdBuff);
            
            
            if(((9 - ((millis()-lastMillis)/60000)) == 0) && ((59 - ((millis()-lastMillisSecond)/1000)) == 0))
            {
              wrongPassword = 0;
              verify = false;
            }
          }
        }
        
        lcd.clear();
        lcd.setCursor(0,0);
        lcd.print("Invalid Password");
        delay(1000);
        lcd.clear();
        tampungKeypad = "";
        counterKeypad = 0;
      }
    }
  }
}
uint8_t deleteFingerprint(uint8_t id) //func untuk delete ID
{
  uint8_t p = -1;
  
  p = finger.deleteModel(id);

  if (p == FINGERPRINT_OK) {
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Deleted!");
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    //Serial.println("Communication error");
    return p;
  } else if (p == FINGERPRINT_BADLOCATION) {
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Couldn't Delete");
    delay(1000);
    return p;
  } else if (p == FINGERPRINT_FLASHERR) {
    //Serial.println("Error writing to flash");
    return p;
  } else {
    //Serial.print("Unknown error: 0x"); Serial.println(p, HEX);
    return p;
  }   
}
void daftarFinger() //func untuk add user
{
  lcd.setCursor(0,0);
  lcd.print("Masukan No ID: ");
  char customKey = customKeypad.getKey(); 
  if(customKey){
    if(customKey == '|')
    {
      tampungKeypad = "";
      lcd.clear();
      daftar = false;
    }
    if(customKey != '~' && customKey != '&' && customKey != '@' &&
       customKey != '#' && customKey != '*' && customKey != '^' &&
       customKey != '>' && customKey != '<' && customKey != 'v'
    )
    {
      lcd.setCursor(counterKeypad,1);
      lcd.print(customKey);
      counterKeypad++;
      tampungKeypad += String(customKey); //simpan inputan dari keypad
    }
    
    else  //jika di tekan enter
    {
      id = tampungKeypad.toInt(); //inputan dari keypad dari bentuk char di ubah ke bentuk integer
      lcd.clear();
      if(id > maxId)  //cek apakah id inputan lebih besar dari nilai maximal ID
      {
        lcd.setCursor(0,0);
        lcd.print("Max ID Is: "); //tampilkan notif
        lcd.print(maxId);
        delay(2000);
        daftar = false;
      }
      else
      {
        while (!  getFingerprintEnroll() ); //panggil func getFingerprintEnroll, yg digunakan untuk menyimpan Fingerprint
      }
    }
  }
}

uint8_t getFingerprintEnroll() //func untuk menyimpan fingerprint
{

  int p = -1;
  lcd.setCursor(0,0);
  lcd.print("Place Finger");
  while (p != FINGERPRINT_OK) {
    p = finger.getImage();
    switch (p) {
    case FINGERPRINT_OK:
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("Image Taken");
      delay(1000);
      break;
    case FINGERPRINT_NOFINGER:
      //Serial.println(".");
      break;
    case FINGERPRINT_PACKETRECIEVEERR:
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("Com error");
      break;
    case FINGERPRINT_IMAGEFAIL:
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("Imaging error");
      break;
    default:
      //Serial.println("Unknown error");
      break;
    }
  }

  // OK success!

  p = finger.image2Tz(1);
  switch (p) {
    case FINGERPRINT_OK:
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("Image Converted");
      delay(1000);
      break;
    case FINGERPRINT_IMAGEMESS:
      //Serial.println("Image too messy");
      return p;
    case FINGERPRINT_PACKETRECIEVEERR:
      //Serial.println("Communication error");
      return p;
    case FINGERPRINT_FEATUREFAIL:
      //Serial.println("Could not find fingerprint features");
      return p;
    case FINGERPRINT_INVALIDIMAGE:
      //Serial.println("Could not find fingerprint features");
      return p;
    default:
      //Serial.println("Unknown error");
      return p;
  }
  
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Remove Finger");
  delay(2000);
  p = 0;
  while (p != FINGERPRINT_NOFINGER) {
    p = finger.getImage();
  }
  Serial.print("ID "); Serial.println(id);
  p = -1;
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Place Same");
  lcd.setCursor(0,1);
  lcd.print("Finger Again");
  while (p != FINGERPRINT_OK) {
    p = finger.getImage();
    switch (p) {
    case FINGERPRINT_OK:
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("Image Taken");
      delay(1000);
      break;
    case FINGERPRINT_NOFINGER:
      //Serial.print(".");
      break;
    case FINGERPRINT_PACKETRECIEVEERR:
      //Serial.println("Communication error");
      break;
    case FINGERPRINT_IMAGEFAIL:
      //Serial.println("Imaging error");
      break;
    default:
      //Serial.println("Unknown error");
      break;
    }
  }

  // OK success!

  p = finger.image2Tz(2);
  switch (p) {
    case FINGERPRINT_OK:
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("Image Converted");
      delay(1000);
      break;
    case FINGERPRINT_IMAGEMESS:
      //Serial.println("Image too messy");
      return p;
    case FINGERPRINT_PACKETRECIEVEERR:
      //Serial.println("Communication error");
      return p;
    case FINGERPRINT_FEATUREFAIL:
      //Serial.println("Could not find fingerprint features");
      return p;
    case FINGERPRINT_INVALIDIMAGE:
      //Serial.println("Could not find fingerprint features");
      return p;
    default:
      //Serial.println("Unknown error");
      return p;
  }
  
  // OK converted!
  //Serial.print("Creating model for #");  Serial.println(id);
  
  p = finger.createModel();
  if (p == FINGERPRINT_OK) {
    //Serial.println("Prints matched!");
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Prints matched!");
    delay(1000);
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    //Serial.println("Communication error");
    return p;
  } else if (p == FINGERPRINT_ENROLLMISMATCH) {
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Prints matched!");
    delay(1000);
    return p;
  } else {
    //Serial.println("Unknown error");
    return p;
  }   
  
  //Serial.print("ID "); Serial.println(id);
  p = finger.storeModel(id);
  if (p == FINGERPRINT_OK) {
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Stored!");
    delay(1000);
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    //Serial.println("Communication error");
    return p;
  } else if (p == FINGERPRINT_BADLOCATION) {
    //Serial.println("Could not store in that location");
    return p;
  } else if (p == FINGERPRINT_FLASHERR) {
    //Serial.println("Error writing to flash");
    return p;
  } else {
    //Serial.println("Unknown error");
    return p;
  }   
}


// returns -1 if failed, otherwise returns ID #
int getFingerprintIDez()  //func untuk cek ID
{
  uint8_t p = finger.getImage();
  if (p != FINGERPRINT_OK)  return -1;

  p = finger.image2Tz();
  if (p != FINGERPRINT_OK)  return -1;

  p = finger.fingerFastSearch();
  if (p == FINGERPRINT_OK) {    //jika data finger di temukan maka...
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Found ID:");
    lcd.print(finger.fingerID);

    if(!verifyFinger) //jika bukan admin maka....
    {
      digitalWrite(redLed, LOW);  //nyalakan led 
      digitalWrite(yelLed, HIGH);
      digitalWrite(valve, HIGH);  //nyalakan buzzer
      delay(5000);
      digitalWrite(yelLed, LOW);
      digitalWrite(redLed, HIGH);
      digitalWrite(valve, LOW);

      String idLogin = String(finger.fingerID);
      mySim.listen();
      atCommand("AT+CMGF=1",1,"OK");  //set mode text untuk sim900
  
      String numberToSend="";
      for(int epromNumber=10; epromNumber<22; epromNumber++)  
      {
        numberToSend += String(EEPROM.read(epromNumber)); //ambil nomer yg telah terseimpan
      }
      
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print(numberToSend);
      delay(2000);
      lcd.clear();
      
      atCommand("AT+CMGS=\"" +numberToSend+ "\"",1,">");//setting nomer tujuan  
      
      atCommand("ID: "+idLogin+ " Is Login",1,">");//isi dari sms
      Serial.println("Mengirim Char Ctrl+Z / ESC untuk keluar dari menu SMS");
      mySim.println((char)26);//ctrl+z untuk keluar dari menginputkan isi sms
      delay(2000); //tahan selama 1 detik

    }
    
    if(finger.fingerID == 1)  //jika yg masuk adalah Admin, admin id = 1
    {
      if(adminLogin)
      {
        verifyFinger = false;
        adminLogin = false; 
      }
      else
      {
        verifyFinger = false;
        menuCRUD = true;
      }
    }
    digitalWrite(buzzer, on); //nyalakan buzzer
    delay(100);
    digitalWrite(buzzer, off);
    delay(20);
    digitalWrite(buzzer, on);
    delay(100);
    digitalWrite(buzzer, off);
    delay(20);


    mySerial.listen();  //switching ke serial mode finger
    
    lcd.clear();
    absens = false;
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    //Serial.println("Communication error");
    return p;
  } else if (p == FINGERPRINT_NOTFOUND) {
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Not Found ");
    digitalWrite(buzzer, on);
    delay(1000);
    digitalWrite(buzzer, off);
    delay(1000);

    wrongFinger++;
    if(wrongFinger > 4)     //jika finger print salah > 4 kali maka...
    {
      long int lastMillisSecond = millis();
      lcd.clear();
      while((millis() - lastMillisSecond) < (1*60000))  //looping sampai 1 menit
      {
        lcd.setCursor(0,0);
        lcd.print("Please Wait...");
        lcd.setCursor(0,1);
        lcd.print("0:");
        long int secondS = 59 - ((millis()-lastMillisSecond)/1000);
        lcd.setCursor(2,1);
        sprintf(lcdBuff,"%2d", secondS);
        lcd.print(lcdBuff);
        
      }
      if((millis() - lastMillisSecond) > 60000)
      {
        wrongFinger = 0;
      }
    }
    
    mySim.listen();
    atCommand("AT+CMGF=1",1,"OK");  //set mode text untuk sim900
    
    String numberToSend="";
    for(int epromNumber=10; epromNumber<22; epromNumber++)
    {
      numberToSend += String(EEPROM.read(epromNumber));
    }
    
    atCommand("AT+CMGS=\"" +numberToSend+ "\"",1,">");//setting nomer tujuan  
        
    atCommand("Danger!!!",1,">");//isi dari sms
    Serial.println("Mengirim Char Ctrl+Z / ESC untuk keluar dari menu SMS");
    mySim.println((char)26);//ctrl+z untuk keluar dari menginputkan isi sms
    delay(2000); //tahan selama 1 detik
    
    mySerial.listen();
    lcd.clear();
    absens = false;
    return p;
  } else {
    //Serial.println("Unknown error");
    return p;
  }   
  
  return finger.fingerID; 
}

//func untuk perintah setting SIM800
void atCommand(String iCommand, int timing, char myText[])  //parsing data digunakan untuk mendapatkan balasan dari sim
{
  String onOff = (String)myText;  //jadikan data array char myText[] menjadi string

  while(timing>maxTime)   //selama waktu timeout belum terpenuhi maka...
  {
    mySim.println(iCommand); //kirim data ke sim800 sesuai dengan perintah
    if(mySim.find(myText)) //jika mikrokontroller menemukan data balasan sesuai dengan yg diinginkan maka perintah sudah terpenuhi
    {
      found = true;
      
      break;
    }
    maxTime++;
  }
  if(found == true)
  {
    autoReset = false;
    
    counterCommand++;
   
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Sukses");
    delay(1000);
  }
  else
  {
      autoReset = true;
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("Error");
      delay(1000);
      
      digitalWrite(resetPin, LOW);
      delay(200);
      digitalWrite(resetPin, HIGH);
      delay(15000);
      counterCommand = 0;
  }
  if(counterCommand >=100)
  {
    counterCommand = 3;
  }
  found = false;
  maxTime=0;
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("End");
  delay(1000);
}
