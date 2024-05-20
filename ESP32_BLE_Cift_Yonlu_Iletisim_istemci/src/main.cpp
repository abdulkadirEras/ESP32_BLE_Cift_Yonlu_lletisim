
//**********************İSTEMCİ**************************

#include <Arduino.h>
#include "BLEDevice.h"
#include <BLE2902.h>
#include <iostream>
#include <string>
using namespace std;

/* Sunucunun Hizmet UUID'si belirtiliyor */
static BLEUUID serviceUUID("4fafc201-1fb5-459e-8fcc-c5c9c331914b");
/* Sunucunun Karakteristik UUID'si belirtiliyor */
static BLEUUID characteristicUUID("beb5483e-36e1-4688-b7f5-ea07361b26a8");


static boolean baglanti_yap = false;
static boolean baglandi = false;
static boolean tarama_yap = false;

static BLERemoteCharacteristic* pRemoteCharacteristic;
static BLEAdvertisedDevice* myDevice;



static void notifyCallback(BLERemoteCharacteristic* pBLERemoteCharacteristic,
                            uint8_t* pData, size_t length, bool isNotify)
{
  uint8_t sayi,veri_sonu_indeksi=0;
  String gelen_veri=(char*)pData;
  bool ac_kapa=0;
  // gelen_veri  stringi içinde '~' karakterin indeksini alma
  for (uint8_t i = 0; i < gelen_veri.length(); i++)
  {
    /* veri sonu bulunacak. indeksi veri_sonu değişkenine atayacak*/
    if(gelen_veri[i]=='~')
    {
      veri_sonu_indeksi=i;
      ac_kapa=1;
    }
  }
  if (ac_kapa==1)
  {
    /* gelen string ifade 0 dan başlayarak veri sonu indeksi numarasına kadar kesiliyor. */
    gelen_veri=gelen_veri.substring(0,veri_sonu_indeksi);
  }


  //gelen verinin ilk indeksi '0' karakter ve integere çevirme fonksiyonu sıfıra eşitse sayı integerdır.
  //integere çevirme fonksiyonu sıfıra eşit değilse yine aynı şekilde sayı integerdır.
  //(integere çevirme fonksiyonu string ifadedeki char ları sıfır olarak döndürüyor)
  if((gelen_veri[0]=='0' && atoi(gelen_veri.c_str())==0) || (atoi(gelen_veri.c_str())!=0))
  {
    Serial.print("istemciye gelen integer veri: ");
    sayi=atoi(gelen_veri.c_str());//string ifadeyi integer ifadeye çevirir
    Serial.println(sayi);
    Serial.println("************************************");
  }
  else
  {
    Serial.print("istemciye gelen string veri: ");
    Serial.println(gelen_veri.c_str());
    Serial.println("************************************");
  }
}

class MyClientCallback : public BLEClientCallbacks
{
  void onConnect(BLEClient* pclient)
  {
    Serial.println("baglantı basarili.");
  }

  void onDisconnect(BLEClient* pclient)
  {
    baglandi = false;
    Serial.println("baglanti kesildi.");
  }
};

/*BLE Sunucusuna bağlantıyı başlatın*/
bool connectToServer()
{
  Serial.print("baglantı yapiliyor: ");
  Serial.println(myDevice->getAddress().toString().c_str());
 
  //istemci olusturuluyor
  BLEClient*  pClient  = BLEDevice::createClient();
  Serial.println(" - istemci olusturuldu");
  pClient->setClientCallbacks(new MyClientCallback());


  /* BLE sunucusuna bağlanılması */
  pClient->connect(myDevice);  // adres yerine BLE Tanıtılan Aygıtı iletirseniz, eş aygıt adresi türü tanınır (genel veya özel)
  Serial.println(" - sunucuya baglandi");

  // Sunucu servis ile bağlantı kurulması
  BLERemoteService* pRemoteService = pClient->getService(serviceUUID);
  if (pRemoteService == nullptr)
  {
    Serial.print("servis UUID bulunamadi: ");
    Serial.println(serviceUUID.toString().c_str());
    pClient->disconnect();
    return false;
  }
  Serial.println(" - servis bulundu.");

  // Sunucu karakteristiği ile bağlantı kurulması
  pRemoteCharacteristic = pRemoteService->getCharacteristic(characteristicUUID);
  if (pRemoteCharacteristic == nullptr)
  {
    Serial.print("UUID Karakteristik bulunamadi: ");
    Serial.println(characteristicUUID.toString().c_str());
    pClient->disconnect();
    return false;
  }
  Serial.println("-karakteristik bulundu");

  baglandi = true;
  return true;
}
/* BLE sunucularını tarayıp aradığımız hizmetin advertising yapan ilk sunucuyu buluyoruz. */
class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks
{
 /* Her advertising BLE sunucusu için çağrılır. */
  void onResult(BLEAdvertisedDevice advertisedDevice)
  {
    Serial.print("Tanıtılan Cihaz bulundu: ");
    Serial.println(advertisedDevice.toString().c_str());

    /* Bir cihaz bulunduğunda, aradığımız hizmeti içerip içermediğine baklıyor. */
    if (advertisedDevice.haveServiceUUID() && advertisedDevice.isAdvertisingService(serviceUUID))
    {
      BLEDevice::getScan()->stop();
      myDevice = new BLEAdvertisedDevice(advertisedDevice);
      baglanti_yap = true;
      tarama_yap = true;

    }
  }
};


void setup()
{
  Serial.begin(9600);
  BLEDevice::init("ESP32-BLE-İSTEMCİ");
 

  /* Bir Tarayıcı alın ve yeni bir cihaz algıladığımızda bilgilendirilmek için kullanmak
  istediğimiz geri aramayı ayarlayın. Aktif tarama istediğimizi belirtin ve taramayı 5 saniye
  boyunca çalışacak şekilde başlatın. */
  BLEScan* pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setInterval(1349);
  pBLEScan->setWindow(449);
  pBLEScan->setActiveScan(true);
  pBLEScan->start(5, false);

}


std::string value;
String new_value;
uint8_t sayac=0;
bool tekrr=0;
bool anahtar=true;
void loop()
{

  /*"baglanti_yap" bayrağı doğruysa, bağlanmak istediğimiz  BLE Sunucusunu taradık ve bulduk.
  Şimdi ona bağlanıyoruz. Bağlandıktan sonra, bağlı bayrağı doğru olarak ayarladık. */
  if (baglanti_yap == true || tekrr==true)//bağlanmak istediğimiz sunucuya bağlantı varsa
  {
    if (connectToServer())//connectToServer() parametre döndüren fonksiyon. 1 ise sunucuya bağlandı değilse bağlanamadı    
    {
      Serial.println("BLE sunucusuna baglanildi.");
      Serial.println("*****BLE İSTEMCİ*****");
      tekrr=0;
    }
    else
    {
      Serial.println("BLE sunucusuna baglanilamadi.");
      tekrr=1;//bağlantı başarısız olursa tekrar bağlantı yapması için
    }
    baglanti_yap = false;
  }

  /* Eş bir BLE Sunucusuna bağlıysak, önyüklemeden bu yana geçerli zamana her ulaşıldığında
  özelliği güncelleyin. */

  //bağlantı varsa veri gönderilir
  if (baglandi)
  {

    if(anahtar==true)
    {
      //VERİ OKUMAK İÇİN
      if(pRemoteCharacteristic->canRead())
      {
        //bildiri kontrol ediliyor
        if(pRemoteCharacteristic->canNotify())
        {
          pRemoteCharacteristic->registerForNotify(notifyCallback);
        }
        anahtar=false;
      }
    }
    else
    {
      //new_value = "sunucu ile haberlesmede geçen süre: " + String(millis()/2000);  
      std::string gonderilen_veri;

      if(sayac%2==1)
      {
        gonderilen_veri= "merhaba sunucu";
      }

      else
      {
        gonderilen_veri= std::to_string(sayac);//std::to_string(sayac)-> integer veriyi string veriye çevirir.
      }
     
      //VERİ GONDERMEK İÇİN
      pRemoteCharacteristic->writeValue(gonderilen_veri.c_str(), gonderilen_veri.length());  
      anahtar=true;
      sayac++;

      gonderilen_veri="";
    }
   
  }
  else if(tarama_yap)
  {
    BLEDevice::getScan()->start(0);  // bu sadece bağlantıyı kestikten sonra taramaya başlatmak için
  }
 

  delay(2000); /* Döngüler arasında iki saniye gecikme */
   
}
