#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h> //segun la libreria se diseño para el esp8266, pero lo admite el esp32

const char *ssid = "";
const char *password = "";

const char *mqtt_server = "udlabrokeriot.tk"; //http://broker.mqtt-dashboard.com/index.html
const int mqtt_port = 1883;                   //TCP_URL
const char *mqtt_user = "testemqx";
const char *mqtt_pass = "public";

TaskHandle_t Task1;

WiFiClient espClient;
PubSubClient client(espClient);

long lastMsg = 0;
char msg[100];

int card = 1;
String dummy;

//*****************************
//*** DECLARACION FUNCIONES ***
//*****************************
void setup_wifi();
void callback(char *topic, byte *payload, unsigned int length);
void reconnect();


//*****************************
//***   SENSOR INT TEMP     ***
//*****************************
#ifdef __cplusplus
extern "C"
{
#endif

  uint8_t temprature_sens_read();

#ifdef __cplusplus
}
#endif

uint8_t temprature_sens_read();



//*****************************
//***   TAREA OTRO NUCLEO   ***
//*****************************
void codeForTask1(void *parameter)
{

  for (;;)
  {
     
    dummy = String((temprature_sens_read() - 32) / 1.8);
    vTaskDelay(10);
  }
}

void setup()
{
  pinMode(BUILTIN_LED, OUTPUT);
  Serial.begin(9600);
  randomSeed(micros());

  xTaskCreatePinnedToCore(
      codeForTask1, /* Task function. */
      "Task_1",     /* name of task. */
      1000,         /* Stack size of task */
      NULL,         /* parameter of the task */
      1,            /* priority of the task */
      &Task1,       /* Task handle to keep track of created task */
      0);           /* Core */

  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
}

void loop()
{
  if (!client.connected())
  {
    reconnect();
  }

  client.loop();

  long now = millis();
  if (now - lastMsg > 1000)
  {
    lastMsg = now;
    card++;

    String to_send = "5ea66203d0794b1b851727a7," + String(card) + "," + dummy;
    to_send.toCharArray(msg, 100);

    char topic[25];
    String topic_aux = "porvenir/porvenir_1";
    topic_aux.toCharArray(topic, 25);

    Serial.print("Publicamos mensaje -> ");
    Serial.println(msg);

    client.publish(topic, msg);
  }
}

//*****************************
//***    CONEXION WIFI      ***
//*****************************
void setup_wifi()
{
  delay(10);
  // Nos conectamos a nuestra red Wifi
  Serial.println();
  Serial.print("Conectando a ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("Conectado a red WiFi!");
  Serial.println("Dirección IP: ");
  Serial.println(WiFi.localIP());
}

void callback(char *topic, byte *payload, unsigned int length)
{
  String incoming = "";
  Serial.print("Mensaje recibido desde -> ");
  Serial.print(topic);
  Serial.println("");
  for (int i = 0; i < length; i++)
  {
    incoming += (char)payload[i];
  }
  incoming.trim();
  Serial.println("Mensaje -> " + incoming);

  if (incoming == "on")
  {
    digitalWrite(BUILTIN_LED, HIGH);
  }
  else
  {
    digitalWrite(BUILTIN_LED, LOW);
  }
}

void reconnect()
{

  while (!client.connected())
  {
    Serial.print("Intentando conexión Mqtt...");
    // Creamos un cliente ID
    String clientId = "esp32_";
    clientId += String(random(0xffff), HEX);
    // Intentamos conectar
    if (client.connect(clientId.c_str(), mqtt_user, mqtt_pass))
    {
      Serial.println("Conectado!");
      digitalWrite(BUILTIN_LED, HIGH);
      // Nos suscribimos
      client.subscribe("test/esp0001/led");
    }
    else
    {
      Serial.print("falló :( con error -> ");
      Serial.print(client.state());
      Serial.println(" Intentamos de nuevo en 5 segundos");

      delay(5000);
    }
  }
}
