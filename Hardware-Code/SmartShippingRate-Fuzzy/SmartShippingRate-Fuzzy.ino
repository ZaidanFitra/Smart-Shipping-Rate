#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"
#include <Fuzzy.h>
#include <HX711_ADC.h>
#if defined(ESP8266)|| defined(ESP32) || defined(AVR)
#include <EEPROM.h>
#endif

void startFirebase();
int ukurDimensi(int trigPin, int echoPin, int platform);
int ukurVolume();
int ukurMassa();
String getData(String jarakPath);

FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;
bool signupOK = false;

#define WIFI_SSID "wifi-ssid"
#define WIFI_PASSWORD "wifi-password"
#define API_KEY "firebase-api-key"
#define DATABASE_URL "database-url"

#define volumePath "pathToVolume"
#define massaPath "pathToMassa"
#define jarakPath "pathToJarak"
#define ongkirPath "pathToOngkosKirim"

#define echoPin1 12
#define trigPin1 13
#define echoPin2 26
#define trigPin2 27
#define echoPin3 32
#define trigPin3 33

#define LED_BUITLIN 2

const int HX711_dout = 16;
const int HX711_sck = 4;   

HX711_ADC LoadCell(HX711_dout, HX711_sck);

const int calVal_eepromAdress = 0;
unsigned long prevTime = 0;
unsigned long prev_sendData = 0;
unsigned long prev_getData = 0;
int prev_weightGram = 0;
int prev_volume = 0;
int prev_massa = 0;
int prev_jarak = 0;
String prev_ongkir = " ";
String stringValue;

// Faktor kalibrasi untuk mengonversi nilai sensor ke gram
float calibrationFactor = 210;

// Fuzzy
Fuzzy *fuzzy = new Fuzzy();

// FuzzyInput Volume
FuzzySet *kecil = new FuzzySet(0, 0, 3000, 6000);
FuzzySet *sedang1 = new FuzzySet(4000, 12000, 12000, 20000);
FuzzySet *besar = new FuzzySet(18000, 21000, 27000, 27000);

// FuzzyInput Massa
FuzzySet *ringan = new FuzzySet(0, 0, 500, 1500);
FuzzySet *sedang2 = new FuzzySet(500, 2750, 2750, 5000);
FuzzySet *berat = new FuzzySet(4000, 5000, 7000, 7000);

// FuzzyInput Jarak
FuzzySet *dekat = new FuzzySet(0, 0, 60, 180);
FuzzySet *sedang3 = new FuzzySet(50, 450, 450, 850);
FuzzySet *jauh = new FuzzySet(720, 840, 1000, 1000);

// FuzzyOutput Ongkos Kirim
FuzzySet *murah = new FuzzySet(0, 0, 8000, 15000);
FuzzySet *sedang4 = new FuzzySet(10000, 18000, 26000, 35000);
FuzzySet *mahal = new FuzzySet(30000, 38000, 46000, 55000);
FuzzySet *sangatmahal = new FuzzySet(50000, 57000, 70000, 70000);

void setup()
{
  Serial.begin(115200);

  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);

  //Set PinMode Sensor Ultrasonik
  pinMode(echoPin1, INPUT);
  pinMode(trigPin1, OUTPUT);
  pinMode(echoPin2, INPUT);
  pinMode(trigPin2, OUTPUT);
  pinMode(echoPin3, INPUT);
  pinMode(trigPin3, OUTPUT);

  // FuzzyInput Volume
  FuzzyInput *volume = new FuzzyInput(1);

  volume->addFuzzySet(kecil);
  volume->addFuzzySet(sedang1);
  volume->addFuzzySet(besar);
  fuzzy->addFuzzyInput(volume);

  // FuzzyInput Massa
  FuzzyInput *massa = new FuzzyInput(2);

  massa->addFuzzySet(ringan);
  massa->addFuzzySet(sedang2);
  massa->addFuzzySet(berat);
  fuzzy->addFuzzyInput(massa);

  // FuzzyInput Jarak
  FuzzyInput *jarak = new FuzzyInput(3);

  jarak->addFuzzySet(dekat);
  jarak->addFuzzySet(sedang3);
  jarak->addFuzzySet(jauh);
  fuzzy->addFuzzyInput(jarak);

  // FuzzyOutput Ongkos Kirim
  FuzzyOutput *ongkosKirim = new FuzzyOutput(1);

  ongkosKirim->addFuzzySet(murah);
  ongkosKirim->addFuzzySet(sedang4);
  ongkosKirim->addFuzzySet(mahal);
  ongkosKirim->addFuzzySet(sangatmahal);
  fuzzy->addFuzzyOutput(ongkosKirim);

  //Volume Kecil dan Massa Ringan
  FuzzyRuleAntecedent *vKecilmRingan = new FuzzyRuleAntecedent();
  vKecilmRingan->joinWithAND(kecil, ringan);

  //Volume Kecil dan Massa Sedang
  FuzzyRuleAntecedent *vKecilmSedang = new FuzzyRuleAntecedent();
  vKecilmSedang->joinWithAND(kecil, sedang2);

  //Volume Kecil dan Massa Berat
  FuzzyRuleAntecedent *vKecilmBerat = new FuzzyRuleAntecedent();
  vKecilmBerat->joinWithAND(kecil, berat);

  //Volume Sedang dan Massa Ringan
  FuzzyRuleAntecedent *vSedangmRingan = new FuzzyRuleAntecedent();
  vSedangmRingan->joinWithAND(sedang1, ringan);

  //Volume Sedang dan Massa Sedang
  FuzzyRuleAntecedent *vSedangmSedang = new FuzzyRuleAntecedent();
  vSedangmSedang->joinWithAND(sedang1, sedang2);

  //Volume Sedang dan Massa Berat
  FuzzyRuleAntecedent *vSedangmBerat = new FuzzyRuleAntecedent();
  vSedangmBerat->joinWithAND(sedang1, berat);

  //Volume Besar dan Massa Ringan
  FuzzyRuleAntecedent *vBesarmRingan = new FuzzyRuleAntecedent();
  vBesarmRingan->joinWithAND(besar, ringan);

  //Volume Besar dan Massa Sedang
  FuzzyRuleAntecedent *vBesarmSedang = new FuzzyRuleAntecedent();
  vBesarmSedang->joinWithAND(besar, sedang2);

  //Volume Besar dan Massa Berat
  FuzzyRuleAntecedent *vBesarmBerat = new FuzzyRuleAntecedent();
  vBesarmBerat->joinWithAND(besar, berat);

  //Jarak Dekat
  FuzzyRuleAntecedent *jDekat = new FuzzyRuleAntecedent();
  jDekat->joinSingle(dekat);

  //Jarak Sedang
  FuzzyRuleAntecedent *jSedang = new FuzzyRuleAntecedent();
  jSedang->joinSingle(sedang3);

  //Jarak Jauh
  FuzzyRuleAntecedent *jJauh = new FuzzyRuleAntecedent();
  jJauh->joinSingle(jauh);

  //Ongkos Kirim Murah
  FuzzyRuleConsequent *oMurah = new FuzzyRuleConsequent();
  oMurah->addOutput(murah);

  //Ongkos Kirim Sedang
  FuzzyRuleConsequent *oSedang = new FuzzyRuleConsequent();
  oSedang->addOutput(sedang4);

  //Ongkos Kirim Mahal
  FuzzyRuleConsequent *oMahal = new FuzzyRuleConsequent();
  oMahal->addOutput(mahal);

  //Ongkos Kirim Sangat Mahal
  FuzzyRuleConsequent *oSangatMahal = new FuzzyRuleConsequent();
  oSangatMahal->addOutput(sangatmahal);

  //====================================== FuzzyRule ======================================

  //1. Volume Kecil, Massa Ringan, dan Jarak Dekat
  FuzzyRuleAntecedent *vKecilmRinganjDekat = new FuzzyRuleAntecedent();
  vKecilmRinganjDekat->joinWithAND(vKecilmRingan, jDekat);

  FuzzyRule *fuzzyRule1 = new FuzzyRule(1, vKecilmRinganjDekat, oMurah);
  fuzzy->addFuzzyRule(fuzzyRule1);

  //2. Volume Kecil, Massa Ringan, dan Jarak Sedang
  FuzzyRuleAntecedent *vKecilmRinganjSedang = new FuzzyRuleAntecedent();
  vKecilmRinganjSedang->joinWithAND(vKecilmRingan, jSedang);

  FuzzyRule *fuzzyRule2 = new FuzzyRule(2, vKecilmRinganjSedang, oSedang);
  fuzzy->addFuzzyRule(fuzzyRule2);

  //3. Volume Kecil, Massa Ringan, dan Jarak Jauh
  FuzzyRuleAntecedent *vKecilmRinganjJauh = new FuzzyRuleAntecedent();
  vKecilmRinganjJauh->joinWithAND(vKecilmRingan, jJauh);

  FuzzyRule *fuzzyRule3 = new FuzzyRule(3, vKecilmRinganjJauh, oMahal);
  fuzzy->addFuzzyRule(fuzzyRule3);

  //4. Volume Kecil, Massa Sedang, dan Jarak Dekat
  FuzzyRuleAntecedent *vKecilmSedangjDekat = new FuzzyRuleAntecedent();
  vKecilmSedangjDekat->joinWithAND(vKecilmSedang, jDekat);

  FuzzyRule *fuzzyRule4 = new FuzzyRule(4, vKecilmSedangjDekat, oSedang);
  fuzzy->addFuzzyRule(fuzzyRule4);

  //5. Volume Kecil, Massa Sedang, dan Jarak Sedang
  FuzzyRuleAntecedent *vKecilmSedangjSedang = new FuzzyRuleAntecedent();
  vKecilmSedangjSedang->joinWithAND(vKecilmSedang, jSedang);

  FuzzyRule *fuzzyRule5 = new FuzzyRule(5, vKecilmSedangjSedang, oSedang);
  fuzzy->addFuzzyRule(fuzzyRule5);

  //6. Volume Kecil, Massa Sedang, dan Jarak Jauh
  FuzzyRuleAntecedent *vKecilmSedangjJauh = new FuzzyRuleAntecedent();
  vKecilmSedangjJauh->joinWithAND(vKecilmSedang, jJauh);

  FuzzyRule *fuzzyRule6 = new FuzzyRule(6, vKecilmSedangjJauh, oMahal);
  fuzzy->addFuzzyRule(fuzzyRule6);

  //7. Volume Kecil, Massa Berat, dan Jarak Dekat
  FuzzyRuleAntecedent *vKecilmBeratjDekat = new FuzzyRuleAntecedent();
  vKecilmBeratjDekat->joinWithAND(vKecilmBerat, jDekat);

  FuzzyRule *fuzzyRule7 = new FuzzyRule(7, vKecilmBeratjDekat, oMahal);
  fuzzy->addFuzzyRule(fuzzyRule7);

  //8. Volume Kecil, Massa Berat, dan Jarak Sedang
  FuzzyRuleAntecedent *vKecilmBeratjSedang = new FuzzyRuleAntecedent();
  vKecilmBeratjSedang->joinWithAND(vKecilmBerat, jSedang);

  FuzzyRule *fuzzyRule8 = new FuzzyRule(8, vKecilmBeratjSedang, oMahal);
  fuzzy->addFuzzyRule(fuzzyRule8);

  //9. Volume Kecil, Massa Berat, dan Jarak Jauh
  FuzzyRuleAntecedent *vKecilmBeratjJauh = new FuzzyRuleAntecedent();
  vKecilmBeratjJauh->joinWithAND(vKecilmBerat, jJauh);

  FuzzyRule *fuzzyRule9 = new FuzzyRule(9, vKecilmBeratjJauh, oSangatMahal);
  fuzzy->addFuzzyRule(fuzzyRule9);

  //10. Volume Sedang, Massa Ringan, dan Jarak Dekat
  FuzzyRuleAntecedent *vSedangmRinganjDekat = new FuzzyRuleAntecedent();
  vSedangmRinganjDekat->joinWithAND(vSedangmRingan, jDekat);

  FuzzyRule *fuzzyRule10 = new FuzzyRule(10, vSedangmRinganjDekat, oSedang);
  fuzzy->addFuzzyRule(fuzzyRule10);

  //11. Volume Sedang, Massa Ringan, dan Jarak Sedang
  FuzzyRuleAntecedent *vSedangmRinganjSedang = new FuzzyRuleAntecedent();
  vSedangmRinganjSedang->joinWithAND(vSedangmRingan, jSedang);

  FuzzyRule *fuzzyRule11 = new FuzzyRule(11, vSedangmRinganjSedang, oSedang);
  fuzzy->addFuzzyRule(fuzzyRule11);

  //12. Volume Sedang, Massa Ringan, dan Jarak Jauh
  FuzzyRuleAntecedent *vSedangmRinganjJauh = new FuzzyRuleAntecedent();
  vSedangmRinganjJauh->joinWithAND(vSedangmRingan, jJauh);

  FuzzyRule *fuzzyRule12 = new FuzzyRule(12, vSedangmRinganjJauh, oMahal);
  fuzzy->addFuzzyRule(fuzzyRule12);

  //13. Volume Sedang, Massa Sedang, dan Jarak Dekat
  FuzzyRuleAntecedent *vSedangmSedangjDekat = new FuzzyRuleAntecedent();
  vSedangmSedangjDekat->joinWithAND(vSedangmSedang, jDekat);

  FuzzyRule *fuzzyRule13 = new FuzzyRule(13, vSedangmSedangjDekat, oSedang);
  fuzzy->addFuzzyRule(fuzzyRule13);

  //14. Volume Sedang, Massa Sedang, dan Jarak Sedang
  FuzzyRuleAntecedent *vSedangmSedangjSedang = new FuzzyRuleAntecedent();
  vSedangmSedangjSedang->joinWithAND(vSedangmSedang, jSedang);

  FuzzyRule *fuzzyRule14 = new FuzzyRule(14, vSedangmSedangjSedang, oSedang);
  fuzzy->addFuzzyRule(fuzzyRule14);

  //15. Volume Sedang, Massa Sedang, dan Jarak Jauh
  FuzzyRuleAntecedent *vSedangmSedangjJauh = new FuzzyRuleAntecedent();
  vSedangmSedangjJauh->joinWithAND(vSedangmSedang, jJauh);

  FuzzyRule *fuzzyRule15 = new FuzzyRule(15, vSedangmSedangjJauh, oMahal);
  fuzzy->addFuzzyRule(fuzzyRule15);

  //16. Volume Sedang, Massa Berat, dan Jarak Dekat
  FuzzyRuleAntecedent *vSedangmBeratjDekat = new FuzzyRuleAntecedent();
  vSedangmBeratjDekat->joinWithAND(vSedangmBerat, jDekat);

  FuzzyRule *fuzzyRule16 = new FuzzyRule(16, vSedangmBeratjDekat, oMahal);
  fuzzy->addFuzzyRule(fuzzyRule16);

  //17. Volume Sedang, Massa Berat, dan Jarak Sedang
  FuzzyRuleAntecedent *vSedangmBeratjSedang = new FuzzyRuleAntecedent();
  vSedangmBeratjSedang->joinWithAND(vSedangmBerat, jSedang);

  FuzzyRule *fuzzyRule17 = new FuzzyRule(17, vSedangmBeratjSedang, oMahal);
  fuzzy->addFuzzyRule(fuzzyRule17);

  //18. Volume Sedang, Massa Berat, dan Jarak Jauh
  FuzzyRuleAntecedent *vSedangmBeratjJauh = new FuzzyRuleAntecedent();
  vSedangmBeratjJauh->joinWithAND(vSedangmBerat, jJauh);

  FuzzyRule *fuzzyRule18 = new FuzzyRule(18, vSedangmBeratjJauh, oSangatMahal);
  fuzzy->addFuzzyRule(fuzzyRule18);

  //19. Volume Besar, Massa Ringan, dan Jarak Dekat
  FuzzyRuleAntecedent *vBesarmRinganjDekat = new FuzzyRuleAntecedent();
  vBesarmRinganjDekat->joinWithAND(vBesarmRingan, jDekat);

  FuzzyRule *fuzzyRule19 = new FuzzyRule(19, vBesarmRinganjDekat, oMahal);
  fuzzy->addFuzzyRule(fuzzyRule19);

  //20. Volume Besar, Massa Ringan, dan Jarak Sedang
  FuzzyRuleAntecedent *vBesarmRinganjSedang = new FuzzyRuleAntecedent();
  vBesarmRinganjSedang->joinWithAND(vBesarmRingan, jSedang);

  FuzzyRule *fuzzyRule20 = new FuzzyRule(20, vBesarmRinganjSedang, oMahal);
  fuzzy->addFuzzyRule(fuzzyRule20);

  //21. Volume Besar, Massa Ringan, dan Jarak Jauh
  FuzzyRuleAntecedent *vBesarmRinganjJauh = new FuzzyRuleAntecedent();
  vBesarmRinganjJauh->joinWithAND(vBesarmRingan, jJauh);

  FuzzyRule *fuzzyRule21 = new FuzzyRule(21, vBesarmRinganjJauh, oSangatMahal);
  fuzzy->addFuzzyRule(fuzzyRule21);

  //22. Volume Besar, Massa Sedang, dan Jarak Dekat
  FuzzyRuleAntecedent *vBesarmSedangjDekat = new FuzzyRuleAntecedent();
  vBesarmSedangjDekat->joinWithAND(vBesarmSedang, jDekat);

  FuzzyRule *fuzzyRule22 = new FuzzyRule(22, vBesarmSedangjDekat, oMahal);
  fuzzy->addFuzzyRule(fuzzyRule22);

  //23. Volume Besar, Massa Sedang, dan Jarak Sedang
  FuzzyRuleAntecedent *vBesarmSedangjSedang = new FuzzyRuleAntecedent();
  vBesarmSedangjSedang->joinWithAND(vBesarmSedang, jSedang);

  FuzzyRule *fuzzyRule23 = new FuzzyRule(23, vBesarmSedangjSedang, oMahal);
  fuzzy->addFuzzyRule(fuzzyRule23);

  //24. Volume Besar, Massa Sedang, dan Jarak Jauh
  FuzzyRuleAntecedent *vBesarmSedangjJauh = new FuzzyRuleAntecedent();
  vBesarmSedangjJauh->joinWithAND(vBesarmSedang, jJauh);

  FuzzyRule *fuzzyRule24 = new FuzzyRule(24, vBesarmSedangjJauh, oSangatMahal);
  fuzzy->addFuzzyRule(fuzzyRule24);

  //25. Volume Besar, Massa Berat, dan Jarak Dekat
  FuzzyRuleAntecedent *vBesarmBeratjDekat = new FuzzyRuleAntecedent();
  vBesarmBeratjDekat->joinWithAND(vBesarmBerat, jDekat);

  FuzzyRule *fuzzyRule25 = new FuzzyRule(25, vBesarmBeratjDekat, oSangatMahal);
  fuzzy->addFuzzyRule(fuzzyRule25);

  //26. Volume Besar, Massa Berat, dan Jarak Sedang
  FuzzyRuleAntecedent *vBesarmBeratjSedang = new FuzzyRuleAntecedent();
  vBesarmBeratjSedang->joinWithAND(vBesarmBerat, jSedang);

  FuzzyRule *fuzzyRule26 = new FuzzyRule(26, vBesarmBeratjSedang, oSangatMahal);
  fuzzy->addFuzzyRule(fuzzyRule26);

  //27. Volume Besar, Massa Berat, dan Jarak Jauh
  FuzzyRuleAntecedent *vBesarmBeratjJauh = new FuzzyRuleAntecedent();
  vBesarmBeratjJauh->joinWithAND(vBesarmBerat, jJauh);

  FuzzyRule *fuzzyRule27 = new FuzzyRule(27, vBesarmBeratjJauh, oSangatMahal);
  fuzzy->addFuzzyRule(fuzzyRule27);

  //=======================================================================================

  Serial.println();
  Serial.println("Starting...");

  //Memanggil fungsi untuk memulai koneksi WiFi dan Firebase
  startFirebase();

  //Memulai Load Cell
  LoadCell.begin();

  unsigned long stabilizingTime = 2000;
  boolean _tare = true;
  LoadCell.start(stabilizingTime, _tare);

  if (LoadCell.getTareTimeoutFlag()) {
    Serial.println("Timeout, check MCU > HX711 wiring and pin designations");
    while (1);
  } 
  else {
    LoadCell.setCalFactor(calibrationFactor);
    Serial.println("Startup is complete");
  }
}

void loop()
{
  //LED menyala menandakan alat siap digunakan
  digitalWrite(LED_BUILTIN, HIGH);

  int panjang, lebar, tinggi, volume;
  
  //Pemanggilan fungsi untuk mengukur dimensi barang
  panjang = ukurDimensi(trigPin1, echoPin1, 24);
  lebar = ukurDimensi(trigPin2, echoPin2, 23);
  tinggi = ukurDimensi(trigPin3, echoPin3, 21);
  volume = panjang * lebar * tinggi;

  //Pemanggilan fungsi untuk mengukur massa
  int massa = ukurMassa();

  //Ambil data jarak dari Firebase 
  String stringJarak = getData(jarakPath);
  stringJarak.replace("\\\"", "");
  //Ubah String jarak menjadi int
  int jarak = atoi(stringJarak.c_str());

  //Proses Fuzzy Logic
  fuzzy->setInput(1, volume);
  fuzzy->setInput(2, massa);
  fuzzy->setInput(3, jarak);
  fuzzy->fuzzify();
  long ongkosKirim = fuzzy->defuzzify(1);

  //Print hasil pengukuran dan Fuzzy Logic ke serial monitor
  Serial.print("Panjang     : ");
  Serial.print(panjang);
  Serial.println(" cm");
  Serial.print("Lebar       : ");
  Serial.print(lebar);
  Serial.println(" cm");
  Serial.print("Tinggi      : ");
  Serial.print(tinggi);
  Serial.println(" cm");
  Serial.print("Volume      : ");
  Serial.print(volume);
  Serial.println(" cm^3");
  Serial.print("Massa       : ");
  Serial.print(massa);
  Serial.println(" gr");
  Serial.print("Jarak       : ");
  Serial.print(jarak);
  Serial.println(" km");
  Serial.print("Ongkos Kirim: Rp");
  Serial.println(ongkosKirim);
  Serial.println();

  //Ubah long ongkos kirim menjadi String
  String ongkir = String(ongkosKirim);

  //Kirim data ke Firebase hanya ketika data berubah
  if(volume != prev_volume || massa != prev_massa || ongkir != prev_ongkir) {
    //Kirim data setiap 1 detik
    if(millis() - prev_sendData >= 1000) {
      //Kirim int volume
      if(Firebase.RTDB.setInt(&fbdo, volumePath, volume)) {   
        prev_volume = volume;
      }
      else{
        Serial.println("Volume FAILED");
        Serial.println("REASON: " + fbdo.errorReason());
      }
      //Kirim int massa
      if(Firebase.RTDB.setInt(&fbdo, massaPath, massa)) {
        prev_massa = massa;
      }
      else{
        Serial.println("Massa FAILED");
        Serial.println("REASON: " + fbdo.errorReason());
      }
      //Kirim String ongkir
      if(Firebase.RTDB.setString(&fbdo, ongkirPath, ongkir)) {  
        prev_ongkir = ongkir;
      }
      else{
        Serial.println("Ongkir FAILED");
        Serial.println("REASON: " + fbdo.errorReason());
      }

      prev_sendData = millis();
    }
  }
}

//Fungsi untuk memulai koneksi WiFi dan Firebase
void startFirebase()
{
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");

  while(WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  
  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();

  Serial.printf("FIrebase Client v%s\n\n", FIREBASE_CLIENT_VERSION);

  config.api_key = API_KEY;

  if(Firebase.signUp(&config, &auth, "", "")) {
    Serial.println("Firebase success");
    signupOK = true;
  }
  else {
    String firebaseErrorMessage = config.signer.signupError.message.c_str();
    Serial.printf("%s\n", firebaseErrorMessage);
  }

  config.database_url = DATABASE_URL;
  config.token_status_callback = tokenStatusCallback;

  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
}

//Fungsi untuk mengukur dimensi barang
int ukurDimensi(int trigPin, int echoPin, int platform)
{
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  long duration = pulseIn(echoPin, HIGH);
  long distance = duration / 58.2;

  if(distance > platform) {
    distance = platform;
  }

  int dimensi = platform - distance;

  return dimensi;
}

//Fungsi untuk mengukur massa barang
int ukurMassa()
{
  static boolean newDataReady = false;
  int weightGram;

  if (LoadCell.update()) newDataReady = true;

  if (newDataReady) {
    weightGram = LoadCell.getData();
    prev_weightGram = weightGram;
    newDataReady = false;
  }
  else {
    weightGram = prev_weightGram;
  }

  return weightGram;
}


//Fungsi untuk mengambil data dari Firebase
String getData(String path)
{
  //Ambil data setiap 1,5 detik
  if(millis() - prev_getData >= 1500){    
    if (Firebase.RTDB.getString(&fbdo, path)) {
      if (fbdo.dataType() == "string") {
        stringValue = fbdo.stringData();
      }
    }
    prev_getData = millis();
  }
  
  return stringValue;
}
