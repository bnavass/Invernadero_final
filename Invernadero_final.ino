#include <WiFi.h>                // Librería para conexión WiFi
#include <WiFiClientSecure.h>     // Librería para conexión segura, necesaria para el bot de Telegram
#include <UniversalTelegramBot.h> // Librería para interactuar con el bot de Telegram
#include <ArduinoJson.h>          // Librería para manejar JSON (datos del bot de Telegram)
#include "ThingSpeak.h"           // Librería para enviar datos al servicio de ThingSpeak
#include "DHT.h"                  // Librería para interactuar con el sensor DHT11 (temperatura y humedad)
#include <LiquidCrystal_I2C.h>    // Librería para manejar pantalla LCD con comunicación I2C

#define pin1 16                   // Pin para el sensor de temperatura y humedad DHT11
#define ledAzul 13                // Pin del LED azul, utilizado para encender/apagar
#define ledRojo 12                // Pin del LED rojo, utilizado para al alarma
#define motionSensor 27           // Pin del sensor de movimiento PIR

// Configuración de red WiFi y credenciales del bot de Telegram
const char* ssid = "iPhone de Begoña";    // Nombre de la red WiFi
const char* password = "bego1234";        // Contraseña de la red WiFi
#define BOTtoken "7672731326:AAFcDHgeDSh1-Nd7828MsX9D3k9WbQz3HXc" // Token del bot de Telegram
#define CHAT_ID "1518143019"              // ID del chat autorizado para interactuar con el bot

// Configuración de canal y clave para enviar datos a ThingSpeak
unsigned long channelID = 2718593;      
const char* WriteAPIKey = "XKN6PG1VDW6UYRV4"; // Clave para escribir en ThingSpeak

// Objetos necesarios para la conexión con Telegram y ThingSpeak
WiFiClientSecure clientTelegram;             // Para conectar con Telegram
UniversalTelegramBot bot(BOTtoken, clientTelegram); // Bot de Telegram
WiFiClient cliente;                          // Para conexión con ThingSpeak

// Configuración del sensor de temperatura y humedad
DHT dht1(pin1, DHT11);                       // Inicializa el sensor DHT en el pin1

// Configuración de la pantalla LCD con dirección I2C
LiquidCrystal_I2C lcd(0x27, 16, 2);          // LCD con dirección 0x27 y dimensiones 16x2

// Variables de estado
bool ledAzulState = false;                   // Estado del LED azul
bool motionDetected = false;                 // Estado del sensor de movimiento (si ha detectado movimiento)

// Variables de temporización para controlar intervalos
unsigned long previousReadMillis = 0;        // Tiempo previo para lectura del sensor
const long readInterval = 15000;             // Intervalo de lectura de datos en ms
unsigned long previousMotionMillis = 0;      // Tiempo previo para enviar mensaje de movimiento
const long motionInterval = 1000;            // Intervalo mínimo entre mensajes de movimiento en ms

// Función para manejar mensajes nuevos del bot
void handleNewMessages(int numNewMessages) {
  for (int i = 0; i < numNewMessages; i++) {          // Recorre los mensajes recibidos
    String chat_id = String(bot.messages[i].chat_id); // Obtiene el ID del chat
    if (chat_id != CHAT_ID) {                         // Verifica si el usuario es autorizado
      bot.sendMessage(chat_id, "Usuario No Autorizado", ""); // Envía mensaje si no es autorizado
      continue;                                       // Continúa con el siguiente mensaje
    }

    String text = bot.messages[i].text;               // Lee el contenido del mensaje

    if (text.equalsIgnoreCase("haz una foto")) {      // Si el mensaje es "haz una foto"
      ledAzulState = true;                            // Cambia el estado del LED azul a encendido
      digitalWrite(ledAzul, HIGH);                    // Enciende el LED azul
      bot.sendMessage(chat_id, "Foto :)", "");        // Envía un mensaje de respuesta
      delay(500);                                     // Espera 500 ms
      digitalWrite(ledAzul, LOW);                     // Apaga el LED azul
    }

    if (text.equalsIgnoreCase("alarma")) {            // Si el mensaje es "alarma"
      digitalWrite(ledRojo, HIGH);                    // Enciende el LED rojo (alarma)
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Alarma");                            // Muestra mensaje en la pantalla LCD
      lcd.setCursor(0, 1);
      lcd.print("encendida!!");
      bot.sendMessage(chat_id, "Alarma encendida", ""); // Informa que la alarma está encendida
    }

    if (text.equalsIgnoreCase("apagar alarma")) {     // Si el mensaje es "apagar alarma"
      digitalWrite(ledRojo, LOW);                     // Apaga el LED rojo (alarma)
      bot.sendMessage(chat_id, "Alarma apagada", ""); // Informa que la alarma está apagada
    }
  
  }
}

// Función de interrupción para detectar movimiento
void IRAM_ATTR detectsMovement() {
  motionDetected = true;                              // Cambia el estado de movimiento a verdadero
}

// Función para leer el sensor DHT y enviar datos a ThingSpeak
void leerdht1() {
  float t1 = dht1.readTemperature();                  // Lee temperatura
  float h1 = dht1.readHumidity();                     // Lee humedad

  // Repite lectura si el sensor falla
  while (isnan(t1) || isnan(h1)) {                    
    Serial.println("Lectura fallida en el sensor DHT11, repitiendo...");
    delay(2000);                                      // Espera 2 segundos y reintenta
    t1 = dht1.readTemperature();
    h1 = dht1.readHumidity();
  }

  // Envía datos de temperatura y humedad a ThingSpeak
  ThingSpeak.setField(1, t1);                         // Establece field 1 como temperatura
  ThingSpeak.setField(2, h1);                         // Establece field 2 como humedad
  if (ThingSpeak.writeFields(channelID, WriteAPIKey) == 200) {
    Serial.println("Datos enviados a ThingSpeak!");
  } else {
    Serial.println("Error al enviar datos a ThingSpeak");
  }

  // Actualiza los valores en la pantalla LCD cada 15 segundos
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Temp: ");
  lcd.print(t1);
  lcd.print("C");
  lcd.setCursor(0, 1);
  lcd.print("Humedad: ");
  lcd.print(h1);
  lcd.print("%");
}

void setup() {
  Serial.begin(115200);                              // Inicializa comunicación 
  pinMode(ledAzul, OUTPUT);                          // Configura LED azul como salida
  pinMode(ledRojo, OUTPUT);                          // Configura LED rojo como salida
  pinMode(motionSensor, INPUT_PULLUP);               // Configura sensor de movimiento como entrada
  digitalWrite(ledAzul, LOW);                        // Asegura que el LED azul esté apagado
  digitalWrite(ledRojo, LOW);                        // Asegura que el LED rojo esté apagado

  attachInterrupt(digitalPinToInterrupt(motionSensor), detectsMovement, RISING); // Configura interrupción para movimiento

  lcd.init();                                        // Inicializa pantalla LCD
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Conectando...");

  // Conectar a WiFi
  Serial.print("Conectando a WiFi: ");
  Serial.println(ssid);
  WiFi.mode(WIFI_STA);                               // Configura el modo de WiFi
  WiFi.begin(ssid, password);                        // Conecta a la red WiFi
  clientTelegram.setCACert(TELEGRAM_CERTIFICATE_ROOT); // Configura certificado de seguridad para Telegram

  while (WiFi.status() != WL_CONNECTED) {            // Espera hasta que se conecte
    Serial.print(".");
    delay(500);
  }
  Serial.println("\nConectado a WiFi");
  Serial.print("Dirección IP: ");
  Serial.println(WiFi.localIP());

  bot.sendMessage(CHAT_ID, "Bot iniciado", "");      // Envía mensaje de inicio al bot
  lcd.clear();
  lcd.print("Bot y WiFi OK");                        // Envía mensaje de inicio a la pantalla

  ThingSpeak.begin(cliente);                         // Inicia comunicación con ThingSpeak
  dht1.begin();                                      // Inicia sensor DHT11
}

void loop() {
  unsigned long currentMillis = millis(); // Almacena el tiempo actual en milisegundos desde que el programa comenzó a ejecutarse. 

  // 1. Comprobar si hay nuevos mensajes en el bot de Telegram.
  int numNewMessages = bot.getUpdates(bot.last_message_received + 1); // Obtiene el número de nuevos mensajes del bot desde el último mensaje recibido.
  
  if (numNewMessages > 0) {  // Si hay mensajes nuevos...
    handleNewMessages(numNewMessages); // Llama a la función para procesar estos mensajes.
  }

  // 2. Detectar movimiento y enviar alerta.
  // Verifica si se ha detectado movimiento y si ha pasado suficiente tiempo desde la última notificación.
  if (motionDetected && (currentMillis - previousMotionMillis >= motionInterval)) {
    bot.sendMessage(CHAT_ID, "Movimiento detectado!!", ""); // Envía un mensaje de alerta al chat de Telegram indicando que se ha detectado movimiento.
    
    previousMotionMillis = currentMillis; // Actualiza el tiempo de la última detección de movimiento para controlar el intervalo de notificaciones.
    
    motionDetected = false; // Restablece la variable `motionDetected` a `false`, para no enviar mensajes repetidos hasta que se detecte un nuevo movimiento.

    // Actualiza la pantalla LCD con el mensaje de "Movimiento detectado".
    lcd.clear();              // Borra cualquier mensaje previo de la pantalla.
    lcd.setCursor(0, 0);      // Posiciona el cursor en la primera fila, primer carácter.
    lcd.print("Movimiento");  // Escribe "Movimiento" en la pantalla LCD.
    lcd.setCursor(0, 1);      // Mueve el cursor a la segunda fila, primer carácter.
    lcd.print("detectado!");  // Escribe "detectado!" en la pantalla LCD.
  }

  // 3. Leer y enviar datos del sensor DHT a ThingSpeak cada 15 segundos.
  // Verifica si ha pasado el tiempo suficiente desde la última lectura y envío de datos.
  if (currentMillis - previousReadMillis >= readInterval) {
    previousReadMillis = currentMillis; // Actualiza el tiempo de la última lectura de datos.

    leerdht1(); // Llama a la función `leerdht1()` para leer los datos de temperatura y humedad del sensor y enviarlos a ThingSpeak.
  }
}