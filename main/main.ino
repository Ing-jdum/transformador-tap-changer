// -------------------------
// Relay Test for Arduino Mega
// -------------------------

// Change this if needed.
// Most Elegoo optocoupled relay modules are ACTIVE LOW.
const bool RELAY_ACTIVE_LOW = true;

const uint8_t relayPins[] = {2, 3, 4, 5, 6};
const uint8_t NUM_RELAYS = sizeof(relayPins) / sizeof(relayPins[0]);

bool relayState[NUM_RELAYS];

void setRelay(uint8_t index, bool on)
{
    relayState[index] = on;

    if (RELAY_ACTIVE_LOW)
    {
        digitalWrite(relayPins[index], on ? LOW : HIGH);
    }
    else
    {
        digitalWrite(relayPins[index], on ? HIGH : LOW);
    }
}

void printStates()
{
    Serial.println();
    Serial.println("Relay states:");

    for (uint8_t i = 0; i < NUM_RELAYS; i++)
    {
        Serial.print("Relay ");
        Serial.print(i + 1);
        Serial.print(": ");
        Serial.println(relayState[i] ? "ON" : "OFF");
    }

    Serial.println();
    Serial.println("Commands:");
    Serial.println(" 1-5 : Toggle relay");
    Serial.println(" a   : Turn all OFF");
    Serial.println(" s   : Show status");
    Serial.print("> ");
}

void setup()
{
    Serial.begin(115200);

    while (!Serial)
    {
        // Wait for Serial (optional on Mega)
    }

    for (uint8_t i = 0; i < NUM_RELAYS; i++)
    {
        pinMode(relayPins[i], OUTPUT);
        setRelay(i, false);   // Start OFF
    }

    Serial.println("Relay Test Ready");
    printStates();
}

void loop()
{
    if (Serial.available())
    {
        char cmd = Serial.read();

        // Ignore newline characters
        if (cmd == '\n' || cmd == '\r')
            return;

        if (cmd >= '1' && cmd <= '5')
        {
            uint8_t relay = cmd - '1';

            relayState[relay] = !relayState[relay];
            setRelay(relay, relayState[relay]);

            Serial.print("Relay ");
            Serial.print(relay + 1);
            Serial.print(" -> ");
            Serial.println(relayState[relay] ? "ON" : "OFF");
        }
        else if (cmd == 'a' || cmd == 'A')
        {
            for (uint8_t i = 0; i < NUM_RELAYS; i++)
                setRelay(i, false);

            Serial.println("All relays OFF.");
        }
        else if (cmd == 's' || cmd == 'S')
        {
            printStates();
            return;
        }
        else
        {
            Serial.println("Unknown command.");
        }

        Serial.print("> ");
    }
}