#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>

class LED_t
{
    private:
        byte pin;
        byte value;

    public:
        LED_t();
        void set_led( byte pin, byte value );
        void init_pins();

        void change( byte value );
};

LED_t::LED_t()
{
    this -> pin = 0;
    this -> value = 0;
}

void LED_t::set_led( byte pin, byte value )
{
    this -> pin = pin;
    this -> value = value;

    init_pins();
}

void LED_t::change( byte value )
{
    int value_to_reach = ( value <= 255 && value >= 0 ) ? ( value * 100 / 255 ) : 0; // VALUE MUST BE : [ 0; 100 ]; SO VALUE : 100 = INTENSITY : 255

    if ( this -> value > value_to_reach )
    {
        while ( this -> value > value_to_reach )
        {
            analogWrite( this -> pin, this -> value );
            delay( 20 );
            this -> value--;
        }
    }
    else if ( this -> value < value_to_reach )
    {
        while ( this -> value < value_to_reach )
        {
            analogWrite( this -> pin, this -> value );
            delay( 20 );
            this -> value++;
        }
    }
}

void LED_t::init_pins()
{
    pinMode( this -> pin, OUTPUT );
}

const char main_page[] PROGMEM = R"rawliteral(
<!DOCTYPE html>

<script> // 0 https://randomnerdtutorials.com/esp8266-nodemcu-web-server-slider-pwm/

    var colors = { "red" : "255,0,0", "green" : "0,255,0", "blue" : "0,0,255", "purple" : "255,0,255", "yellow" : "255,128,0", "aqua" : "0,255,255", "white" : "255,255,255", "off" : "0,0,0" };

    function load_buttons()
    {
        for ( color in colors )
        {
            document.write( '<input id = "' + color + '" class = "button" type = "button" onclick = "color_button( this )" value = "' + color.toUpperCase() + '" style = "background-color: rgb( ' + colors[color] + ' );">' );
        }
    }

    function change_value( element )
    {
        var value = document.getElementById( element.id ).value;
        document.getElementById( element.id + "_value" ).innerHTML = value;
        var xhr = new XMLHttpRequest();
        xhr.open( "GET", "/slider?" + element.id + "_value=" + value, true );
        xhr.send();
    }

    function color_button( element )
    {
        var xhr = new XMLHttpRequest();
        xhr.open( "GET", "/button?" + "value=" + colors[element.id], true );
        xhr.send();
    }

</script>

<html>
    <head>
        <title>LED</title>
    </head>
    <body>
        <h1>Colors Intensity</h1>
        <div class = "colors" >
            <a>Red</a><br>
            <a id = "red_value" >%RED_value%</a>
            <p><input style="width: 512px;" id = "red" type = "range" onchange = "change_value( this )" min = "0" max = "255" value = "%RED_value%" step = "1" > <!-- 0 -->
        </div>
        <div class = "colors" >
            <a>Green</a><br>
            <a id = "green_value" >%GREEN_value%</a>
            <p><input style="width: 512px;" id = "green" type = "range" onchange = "change_value( this )" min = "0" max = "255" value = "%GREEN_value%" step = "1" > <!-- 0 -->
        </div>
        <div class = "colors" >
            <a>Blue</a><br>
            <a id = "blue_value" >%BLUE_value%<a>
            <p><input style="width: 512px;" id = "blue" type = "range" onchange = "change_value( this )" min = "0" max = "255" value = "%BLUE_value%" step = "1" > <!-- 0 -->
        </div>

        <h1>Colors</h1>
        <div class = "color_buttons" >
            <script> load_buttons(); </script>
        </div>
    </body>
    <footer>
        <br><br>
        <a href = "https://github.com/BlackWolf4k/Led/blob/main/Led.ino" target = "_blank" >Codice</a>
    </footer>
</html>
<style>
    h1
    {
        text-align: center;
    }

    .color_buttons
    {
        display: inline-block;
        width: 50%;
        margin: 0 25%;
    }

    .button
    {
        margin-top: 20px;
        margin-left: 20px;
        color: black;
        width: 200px;
        height: 100px;
        padding: 32px 32px;
        text-align: center;
        font-size: 16px;
        border: 2px solid black;
    }

    .colors
    {
        text-align: center;
        height: 33%;
        font-size: 48px;
    }

    footer, a
    {
        text-align: center;
        text-decoration: none;
        color: black;
    }

</style>
)rawliteral";

const char* ssid = "LED";
AsyncWebServer server( 80 );

String RED_value = "0";
String GREEN_value = "0";
String BLUE_value = "0";

LED_t RED_LED;
LED_t GREEN_LED;
LED_t BLUE_LED;

String processor( const String& str ){
    if ( str == "RED_value" )
        return RED_value;
    else if ( str == "GREEN_value" )
        return GREEN_value;
    else if ( str == "BLUE_value" )
        return BLUE_value;
}

void get_intensity( String str )
{
    int commas[2] = { 0 };
    int string_length = str.length();

    for ( int i = 0, counter = 0; i < string_length; i++ )
    {
        if ( str[i] == ',' )
        {
            commas[counter] = i;
            counter++;
        }
    }

    RED_value = str.substring( 0, commas[0] );
    GREEN_value = str.substring( commas[0] + 1, commas[1] );
    BLUE_value = str.substring( commas[1] + 1 );
}

void change_all( byte red, byte green, byte blue )
{
    RED_LED.change( red );
    GREEN_LED.change( green );
    BLUE_LED.change( blue );
}

byte value( String str )
{
    return (byte)str.toInt();
}

void elaborate_animation( String animation_string )
{
    int str_length = animation_string.length();
    int parts = 0;
    
    int old = 0;
    int next = 0;
    while ( animation_string[next] != ';' || next >= str_length )
        next++;

    for ( int i = 0; i < str_length; i++ )
        if ( animation_string[i] == ';' )
            parts += 1;
    
    String colors[ parts / 2 ] = { "" };
    String delays[ parts / 2 ] = { "" };

    for ( int i = 0; i < parts; i++ )
    {
        if ( i % 2 == 0 )
            colors[i] = animation_string.substring( old, next );
        else
            delays[i] = animation_string.substring( old, next );

        old = next;
        next++;
        while ( animation_string[next] != ';' || next >= str_length )
            next++;
    }

    for ( int i = 0; i < parts; i += 2 )
    {
        if ( colors[i] != "-" )
        {
            get_intensity( colors[i] );
            change_all( value( RED_value ), value( GREEN_value ), value( BLUE_value ) );
        }
        i++;

        if ( delays[i] == "-" )
        {
            change_all( value( RED_value ) - 10, value( GREEN_value ) - 10, value( BLUE_value ) - 10 );
            delay( 500 );
        }
        else
            delay( delays[i].toInt() * 1000 );
    }
}

void setup()
{
    // Start Access Point
    WiFi.softAP( ssid );

    // Init leds
    RED_LED.set_led( 12, 0 ); // D6
    GREEN_LED.set_led( 14, 0 ); // D5
    BLUE_LED.set_led( 4, 0 ); // D2

    // Start Server
    server.on( "/", HTTP_GET, []( AsyncWebServerRequest *request ){ request->send_P( 200, "text/html", main_page, processor ); } );

    // Handle slider requests
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
        else if ( request -> hasParam( "blue_value" ) )
        {
            BLUE_value = request -> getParam( "blue_value" ) -> value();
            BLUE_LED.change( BLUE_value.toInt() );
        }
        request->send_P( 200, "text/html", main_page, processor );
    } );

    // Handle colors buttons requestes
    server.on( "/button", HTTP_GET, []( AsyncWebServerRequest *request )
    {
        if ( request -> hasParam( "value" ) )
        {
            String temp_value = request -> getParam( "value" ) -> value();
            get_intensity( temp_value );
            change_all( value( RED_value ), value( GREEN_value ), value( BLUE_value ) );
        }
        request->send_P( 200, "text/html", main_page, processor );
    } );

    // Handle animations requestes
    server.on( "/animation", HTTP_GET, []( AsyncWebServerRequest *request )
    {
        if ( request -> hasParam( "string" ) )
        {
            String temp_value = request -> getParam( "string" ) -> value();
            elaborate_animation( temp_value );
        }
        request->send_P( 200, "text/html", main_page, processor );
    } );

    server.begin();
}

void loop()
{
}
