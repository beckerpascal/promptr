using System;

namespace PromptrLib
{

    /*
     * Class for handling the hardware calls to the Hue Bridge
     */
    public class ConnectorClient : IConnectorClient
    {
        // private class member to keep the _connection during the whole life time
        private Connection _connection;

        /*
         * Constructor which will connect during the Hue Bridge while initializing
         */
        public ConnectorClient()
        {
            _connection = new ConnectionFactory(Constants.APPKEY).GetConnection();
        }

        /*
         * Turn all connected bulbs on 
         */

        public async void TurnOn()
        {
            await _connection.TurnOn();
        }

        /* 
         * Turn all connect bulbs with a specific color on
         * @param color: in hex format with hastag
         */
        public async void TurnOn(string color)
        {
            await _connection.TurnOn(color);
        }

        /*
         * Turn specific bulb on
         * @param id: values starting with 1 not 0
         */ 
        public async void TurnOn(int id)
        {
            await _connection.TurnOn(id);
        }

        /* 
         * Turn specific bulb with specific color on
         * @param color: in hex format
         * @param id: starting at 1 not 0
         */ 
        public async void TurnOn(string color, int id)
        {
            await _connection.TurnOn(color, id);
        }

        /*
         * Turns all lamps of
         */ 
        public async void TurnOff()
        {
            await _connection.TurnOff();
        }

        /*
         * Turns of a specific id
         */ 
        public async void TurnOff(int id)
        {
            await _connection.TurnOff(id);
        }


        /*
         * Blink with a specific id
         */ 
        public async void Blink(int id)
        {
            await _connection.Blink(id);
        }

        /*
         * Blink with all bulbs
         */ 
        public async void Blink()
        {
            await _connection.Blink();
        }

        /*
         * Fades to the color specified with the percentage inbetween start and end color
         * for a specific bulb
         * @param percent: value between 0 and 100
         * @param startColor: start color in hex format
         * @param endColor: end color in hex format
         * @param id: id of specific bulb
         */ 
        public async void Fade(int percent, string startColor, string endColor, int id)
        {
            await _connection.Fade(percent, startColor, endColor, id);
        }

        /*
         * Fades for a given timeSpan from start to end color with a specific bulb
         * @param timeSpan: timespan for fading         
         * @param startColor: start color in hex format
         * @param endColor: end color in hex format
         * @param id: id of specific bulb
         */ 
        public async void Fade(TimeSpan timeSpan, string startColor, string endColor, int id)
        {
            await _connection.Fade(timeSpan, startColor, endColor, id);
        }
    }
}
