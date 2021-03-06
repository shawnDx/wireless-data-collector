#include <SPI.h>
#include "nRF24L01.h"
#include "RF24.h"
#include "printf.h"
#include "setting.h"
#include "buzzer.h"
#include "radio.h"
#include "dht11.h"

static uint32_t message_count = 0;
bool sendData = false;
bool successFlag = false;
bool errorflag = false;
int tempNHum = 0;

/**
 * Sensor pin definition
 */
#define DHT11PIN 4
dht11 DHT11;

void setup()
{
    Serial.begin(115200);
    printf_begin();
    buzInit(10);
    // Setup and configure rf radio
    while (!radio.begin())
    {
        beep(BEEP_TIME_INTERVAL, ERROR_BEEP); // Indicate radio error
    }

    radio.setPALevel(RF24_PA_MAX);
    radio.setDataRate(RF24_250KBPS);
    radio.enableAckPayload();      // We will be using the Ack Payload feature
    radio.enableDynamicPayloads(); // Ack payloads are dynamic payloads

    // Open pipes to other node for communication
    radio.openWritingPipe(CONTROLLER_ADDRESS);
    radio.openReadingPipe(1, NODE_ADDRESS);
    radio.startListening();
    radio.writeAckPayload(1, &message_count, sizeof(message_count)); // Add an ack packet for the next time around.  This is a simple
    ++message_count;

    radio.printDetails(); // Dump the configuration of the rf unit for debugging
    delay(50);
    attachInterrupt(0, checkRadio, LOW); // Attach interrupt handler to interrupt #0 (using pin 2) on BOTH the sender and receiver
    beep(BEEP_TIME_INTERVAL, INFO_BEEP);
}

/********************** Main Loop *********************/
void loop()
{
    tempNHum = DHT11.read(DHT11PIN);

    if (successFlag)
    {
        // Idicate successfully send the data;
        beep(BEEP_TIME_INTERVAL, SUCCESS_BEEP);
        successFlag = false;
    }
    else if (errorflag)
    {
        // Idicate Tere is an error sending failed
        beep(BEEP_TIME_INTERVAL, ERROR_BEEP);
        errorflag = false;
    }
}

void checkRadio(void)
{

    bool tx, fail, rx;
    radio.whatHappened(tx, fail, rx); // What happened to trigger interrupt?

    if (tx)
    {
        // Have we successfully transmitted request?
        Serial.println(F("Send:OK"));
        successFlag = true;
    }

    if (fail)
    {
        // Have we failed to transmit request?
        // Try to resend data again if we fails
        Serial.println(F("Send:Failed"));
        errorflag = true;
    }

    if (rx || radio.available())
    {
        // Did we receive data  request form centrall ?
        Serial.println("data request received");
        uint8_t code; // Get the request code
        radio.read(&code, sizeof(code));
        Serial.println(code);
        char data[BUFF_SIZE];
        if (code == REQUEST_CODE)
        {
            strcpy(data, getData().c_str());
            radio.writeAckPayload(1, &data, sizeof(data));
        }
        else
        {
            Serial.println(F("Invalid request"));
            beep(BEEP_TIME_INTERVAL, ERROR_BEEP);
        }
    }
}

String getData()
{
    String massage = getHum() + "&" + getTemp() + "&" + String(voltageTest());
    Serial.println(massage);
    return massage;
}

uint16_t voltageTest()
{
    return analogRead(A3);
}

String getTemp()
{
    return String((float)DHT11.temperature);
}

String getHum()
{

    return String((float)DHT11.humidity);
}