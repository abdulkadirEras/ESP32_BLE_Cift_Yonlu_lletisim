//******************************SUNUCU*******************
#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>

//UUID'ler aşağıdaki web sitesinden alınabilir:
// https://www.uuidgenerator.net/

//servis, karakteristik ve tanımlayıcıların UUID si makro tanımlama ile yapıldı
#define SERVICE_UUID        BLEUUID("4fafc201-1fb5-459e-8fcc-c5c9c331914b")
#define CharacteristicUUID  BLEUUID("beb5483e-36e1-4688-b7f5-ea07361b26a8")

//servis ve diğer yapılar için değişkenler tanımlıyoruz. Bunları global olarak tanımlıyoruz:
BLEServer *pServer;
BLEService *pService;
BLECharacteristic *pCharacteristic;



//diğer ESP32 ile baglantı kurulmasını kontrol etmek için yazılan global değişkenler
bool deviceConnected=false;
bool oldDeviceConnected = false;

/*
Server’a bağlantı gerçekleştiğinde haberdar olmak için bir Callback sınıfı tanımlıyoruz.
Bu sınıf ile bağlantı için istediğimiz hız parametrelerini de İstemci (Client) cihazına bildireceğiz.
Bunu yapabilmek için bağlantının kurulmuş olması gerekli
*/

class baglantiCallBack : public BLEServerCallbacks
{
  void onConnect(BLEServer* pServer, esp_ble_gatts_cb_param_t *param)
  {
    //Aşağıdaki kodlar ile bağlantı yapan cihazın adresi öğrenilebilir
    //Serial.print("Baglandi. ID: ");
    //Serial.println(param->connect.conn_id, DEC);
    //Serial.print(" client addr: ");
    //Serial.printf(ESP_BD_ADDR_STR
    //, (uint8_t)param->connect.remote_bda[0]
    //, (uint8_t)param->connect.remote_bda[1]
    //, (uint8_t)param->connect.remote_bda[2]
    //, (uint8_t)param->connect.remote_bda[3]
    //, (uint8_t)param->connect.remote_bda[4]
    //, (uint8_t)param->connect.remote_bda[5] );
    //Serial.println();
   
    //pServer->updateConnParams( //Bağlantı parametrelerini ayarlıyoruz. BKZ: "Dikkat Edilmesi Gereken Noktalar"
    //param->connect.remote_bda,
    //10, 20, 0, 500);
    deviceConnected=true;
  }


  void onDisconnect(BLEServer *pServer)
  {
    deviceConnected = false;
    Serial.println("baglanilamadi.");
  }

};


void setup() {

//uart için baud rate 115200 olarak ayarlanadı ve uart başlatıldı.
Serial.begin(115200);

//BLE cihaza isim verildi ve BLE cihaz başlatıldı.
BLEDevice::init("ESP32-BLE");
pServer = BLEDevice::createServer();
pServer->setCallbacks(new baglantiCallBack());
Serial.println("*****BLE SUNUCU*****");

//servisi oluştur
pService = pServer->createService(SERVICE_UUID);

//karakteristiği oluştur. sunucuya okuma, yazma ve bildirim özellikleri tanımlanıyor.
pCharacteristic = pService->createCharacteristic(
CharacteristicUUID,
BLECharacteristic::PROPERTY_READ   |
BLECharacteristic::PROPERTY_WRITE  |
BLECharacteristic::PROPERTY_NOTIFY
);


//servisi başlat
pService->start();

//Sunucunun duyuru yapmasını sağla
BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
pAdvertising->addServiceUUID(SERVICE_UUID);
pAdvertising->setScanResponse(true);
pAdvertising->setMinPreferred(0x06);  // iPhone bağlantı sorununa yardımcı olan işlevler
pAdvertising->setMinPreferred(0x12);
pServer->getAdvertising()->start();

}




void sendSomeDataBLE(uint8_t *mesaj, int mesaj_boyutu) {
        uint8_t txDeger = 0;

        while (txDeger < mesaj_boyutu)
        {
          pCharacteristic->setValue(&mesaj[txDeger], 1);
          pCharacteristic->notify();
          txDeger++;
          delay(100); // Çok fazla paket gönderilirse, Bluetooth yığını tıkanıklığa girecek
        }
}



//veri alma
//Alınan değeri bir dizeyle karşılaştırmak için işlevi tanımlayın
bool compareData(std::string received, std::string predefined) {
  int receivedLength = received.length();
  int predefinedLength = predefined.length();

  if ((receivedLength / 2) != predefinedLength) {
    return false;
  }

  for (int i = 0; i < predefinedLength; i++) {
    if (received[i * 2] != predefined[i]) {
      return false;
    }
  }

  return true;
}
//Geri arama işlevi aracılığıyla alınan tüm verileri yakalayın
class MyCallbacks: public BLECharacteristicCallbacks
{
    void onWrite(BLECharacteristic *pCharacteristic)
    {
      std::string rxValue = pCharacteristic->getValue();
      std::string lwnCommand = "lwn";
      if (compareData(rxValue, lwnCommand)) {
        Serial.println("lwn command received");
      }
   }
};





uint8_t sayac = 1;
uint8_t sayi=0;
bool anahtar=true;

void loop()
{
    //ESP32 ile bağlantı yapılığı kontrol ediliyor.
    if (deviceConnected)
    {
      if(anahtar==true)
      {
       
        std::string gonderilen_veri;
        //VERi GÖNDERME
        if(sayac%2==1)
        {
          gonderilen_veri="merhaba istemci";
        }
        else
        {
          gonderilen_veri=std::to_string(sayac+3);//std::to_string()-> integer değeri string değere çevirir
        }
         
        /* string ifadenin en sonuna '~' karakteri ekleniyor */
        gonderilen_veri.push_back('~');
       
        pCharacteristic->setValue(gonderilen_veri);
        pCharacteristic->notify(); //Bildirim yollar
       
        anahtar=false;
      }
      else
      {
        //VERİ ALMA
        std::string gelen_veri;
        gelen_veri=pCharacteristic->getValue();
        //gelen verinin ilk indeksi '0' karakter ve integere çevirme fonksiyonu sıfıra eşitse sayı integerdır.
        //integere çevirme fonksiyonu sıfıra eşit değilse yine aynı şekilde sayı integerdır.
        //(integere çevirme fonksiyonu string ifadedeki char ları sıfır olarak döndürüyor)
        if((gelen_veri[0]=='0' && atoi(gelen_veri.c_str())==0) || (atoi(gelen_veri.c_str())!=0))
        {
          Serial.print("sunucuya gelen integer veri: ");
          sayi=atoi(gelen_veri.c_str());//atoi()->string ifadeyi integer ifadeye çevirir
          Serial.println(sayi);
          Serial.println("*****************************");
        }
        else
        {
          Serial.print("sunucuya gelen string veri: ");
          Serial.println(gelen_veri.c_str());
          Serial.println("*****************************");
         
        }
        gelen_veri="";//belirli bir süre sonra aynı veri tekrar gelmeye başlamasını engellemek için
       
       
        /*FLOATA ÇEVİRME*/
        //std::stof() -> STRİNGTEN FLOATA ÇEVİRİR
        //std::stod() -> STRİNGTEN DOUBLE ÇEVİRİR
        //std::stold() -> STRİNGTEN LONG DOUBLE  ÇEVİRİR

        /* ÖRNEK KULLANIM*/
        /*
         
              std::string str = "123.4567";

              // stringten floata çevirme
              float num_float = std::stof(gelen_veri.c_str());

              // stringten double çevirme
              double num_double = std::stod(gelen_veri.c_str());
        */

         sayac++;
         anahtar=true;
      }
    }

    // bağlantı kesildi ise
    if (!deviceConnected && oldDeviceConnected)
    {
        delay(500); // BLE stabil olarak çalışması için gecikme
        pServer->startAdvertising(); // tekrar advertising yapılıyor.
        Serial.println("start advertising");
        oldDeviceConnected = deviceConnected;
    }

    // bağlantı yapılıyor ise
    if (deviceConnected && !oldDeviceConnected)
    {
        // do stuff here on connecting
        oldDeviceConnected = deviceConnected;
    }
     delay(2000);
}
