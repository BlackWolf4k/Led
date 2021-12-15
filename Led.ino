#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>

#define BLUE_PIN 15
#define PULSE_TIMED 10
#define MAX 10

class LED_t
{
    private:
        int CS;
        int INC;
        int UD;
        int value;

    private:
        void change_potentiometer_value();
        void down(){ digitalWrite( this -> UD, 0 ); }
        void up(){ digitalWrite( this -> UD, 1 ); }

    public:
        LED_t();
        void set_led( int CS, int INC, int UD );
        void set_value( int intensity ){ this -> value = ( intensity <= 255 && intensity >= 0 ) ? ( intensity * 100 / 255 ) : 0; }; // VALUE MUST BE : [ 0; 100 ]; SO VALUE : 100 = INTENSITY : 255
        void turn_off();
        void turn_on();
        void change( int value ){ turn_off(); if ( value ) { set_value( value ); turn_on(); } }

        void init_pins();
};

void LED_t::change_potentiometer_value()
{
    digitalWrite( this -> INC, HIGH );
    delay( PULSE_TIMED );
    digitalWrite( this -> CS, LOW );
    digitalWrite( this -> INC, LOW );
    delay( PULSE_TIMED );
    digitalWrite( this -> CS, HIGH );
}

LED_t::LED_t()
{
    this -> CS = 0;
    this -> INC = 0;
    this -> UD = 0;
    this -> value = 0;
}

void LED_t::set_led( int CS, int INC, int UD )
{
    this -> CS = CS;
    this -> INC = INC;
    this -> UD = UD;
    this -> value = 0;
}

void LED_t::turn_off()
{
    up();
    for ( int i = 0; i < MAX; i++ )
    {
        change_potentiometer_value();
        delay( 20 );
    }
}

void LED_t::turn_on()
{
    down();
    for ( int i = 0; i < this -> value; i++ )
    {
        change_potentiometer_value();
        delay( 20 );
    }
}

void LED_t::init_pins()
{
    pinMode( this -> CS, OUTPUT );
    digitalWrite( this -> CS, HIGH );
    pinMode( this -> UD, OUTPUT );
    pinMode( this -> INC, OUTPUT );
    digitalWrite( this -> CS, HIGH );
}

const char main_page[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
    <head>
        <title>LED</title>
    </head>
    <body>
        <h1>Colors Intensity</h1>

        <div class = "colors" >
            <a>Red</a><br>
            <a id = "red_value" >%RED_value%</a>
            <p><input id = "red" type = "range" onchange="change_value( this )" min = "0" max = "255" value = "%RED_value%" step = "1" > <!-- 0 -->
        </div>

        <div class = "colors" >
            <a>Green</a><br>
            <a id = "green_value" >%GREEN_value%</a>
            <p><input id = "green" type = "range" onchange="change_value( this )" min = "0" max = "255" value = "%GREEN_value%" step = "1" > <!-- 0 -->
        </div>

        <div class = "colors" >
            <a>Blue</a><br>
            <a id = "blue_value" >%BLUE_value%<a>
            <p><input id = "blue" type = "checkbox" onchange = "change_relay( this )" ></p>
        </div>
    </body>
</html>

<style>
    .colors
    {
        text-align: center;
        display: inline-block;
        width: 33%;
    }

    h1
    {
        text-align: center;
    }
</style>

<script> // 0 https://randomnerdtutorials.com/esp8266-nodemcu-web-server-slider-pwm/
    function change_value( element )
    {
        var value = document.getElementById( element.id ).value;
        document.getElementById( element.id + "_value" ).innerHTML = value;
        var xhr = new XMLHttpRequest();
        xhr.open( "GET", "/slider?" + element.id + "_value=" + value, true );
        xhr.send();
    }

    function change_relay( element )
    {
        var xhr = new XMLHttpRequest();
        if ( element.checked )
            xhr.open( "GET", "/relay?" + element.id + "_value=" + 1, true );
        else
            xhr.open( "GET", "/relay?" + element.id + "_value=" + 0, true );
        xhr.send();
    }
</script>
)rawliteral";

const char* ssid = "LED";
AsyncWebServer server( 80 );

String RED_value = "0";
String GREEN_value = "0";
String BLUE_value = "0";

LED_t RED_LED;
LED_t GREEN_LED;
//LED_t BLUE_LED;

// 0
String processor( const String& str ){
    if ( str == "RED_value" )
        return RED_value;
    else if ( str == "GREEN_value" )
        return GREEN_value;
    else if ( str == "BLUE_value" )
        return BLUE_value;
}

void setup()
{
    // Start Access Point
    WiFi.softAP( ssid );

    // Init leds
    RED_LED.set_led( 16, 5, 4 ); // D0 - D1 - D2
    RED_LED.init_pins();
    RED_LED.turn_off();

    GREEN_LED.set_led( 0, 2, 14 ); // D3 - D4 - D5
    GREEN_LED.init_pins();
    GREEN_LED.turn_off();

    pinMode( BLUE_PIN, OUTPUT ); // relay

    // BLUE_LED.set_led( , , ); // D6 - D7 - D8
    // BLUE_LED.init_pins();

    // Start Server
    server.on( "/", HTTP_GET, []( AsyncWebServerRequest *request ){ request->send_P( 200, "text/html", main_page, processor ); } );

    // 0
    server.on( "/slider", HTTP_GET, []( AsyncWebServerRequest *request )
    {
        if ( request -> hasParam( "red_value" ) )
        {
            RED_value = request -> getParam( "red_value" ) -> value();
            RED_LED.change( RED_value.toInt() );
        }
        else if ( request -> hasParam( "green_value" ) )
        {
            GREEN_value = request -> getParam( "green_value" ) -> value();
            GREEN_LED.change( GREEN_value.toInt() );
        }
        /*else if ( request -> hasParam( "blue_value" ) )
        {
            BLUE_value = request -> getParam( "blue_value" ) -> value();
            BLUE_LED.change( BLUE_value.toInt() );
        }*/
        request->send_P( 200, "text/html", main_page, processor );
    } );

    server.on( "/relay", HTTP_GET, []( AsyncWebServerRequest *request )
    {
        if ( request -> hasParam( "blue_value" ) )
        {
            BLUE_value = request -> getParam( "blue_value" ) -> value();
            digitalWrite( BLUE_PIN, BLUE_value.toInt() );
        }
        request->send_P( 200, "text/html", main_page, processor );
    } );

    server.begin();
}

void loop()
{
}