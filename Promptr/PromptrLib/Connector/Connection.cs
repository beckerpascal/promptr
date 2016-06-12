using System;
using System.Collections.Generic;
using System.Threading.Tasks;
using PromptrLib.Connector;
using Q42.HueApi;
using Q42.HueApi.Interfaces;

namespace PromptrLib
{
    /*
     * Class for the handling the calls between the Promptr library and the Hue hardware
     */ 
    public class Connection
    {
        private ILocalHueClient client;

        /*
         * Constructor for a specific client
         */ 
        public Connection(ILocalHueClient client)
        {
            this.client = client;
        }

        /*
         * Turn on all bulbs
         */ 
        public async Task TurnOn()
        {
            var command = new LightCommand();
            command.TurnOn();

            await SendCommand(command);
        }

        /*
         * Turn on all bulbs with a specific color
         * @param color: color in hex format
         */ 
        public async Task TurnOn(string color)
        {
            var command = new LightCommand();
            command.SetColor(color);
            command.TurnOn();

            await SendCommand(command);
        }

        /*
         * Turn on a specific bulb
         * @param id: specific bulb id
         */ 
        public async Task TurnOn(int id)
        {
            var command = new LightCommand();
            command.TurnOn();

            await SendCommand(command, new List<string> { id.ToString() });
        }

        /*
         * Turn on a specific bulb with a specific color
         * @param color: color in hex format
         * @param id: id of the bulb
         */ 
        public async Task TurnOn(string color, int id)
        {
            var command = new LightCommand();
            command.SetColor(color);
            command.TurnOn();

            await SendCommand(command, new List<string> { id.ToString() });
        }

        /*
         * Turns off all bulbs
         */ 
        public async Task TurnOff()
        {
            var command = new LightCommand();
            command.TurnOff();

            await SendCommand(command);
        }

        /*
         * Turns off a specific bulb
         * @param id: id of bulb
         */ 
        public async Task TurnOff(int id)
        {
            var command = new LightCommand();
            command.TurnOff();

            await SendCommand(command, new List<string> { id.ToString() });
        }

        /*
         * Fades to the color specified with the percentage inbetween start and end color
         * for a specific bulb
         * @param percent: value between 0 and 100
         * @param startColor: start color in hex format
         * @param endColor: end color in hex format
         * @param id: id of specific bulb
         */
        public async Task Fade(int percent, string startColor, string endColor, int id)
        {
            ColorCalculation colorCalc = new ColorCalculation();
            var command = new LightCommand();

            command.SetColor(colorCalc.CalculateColorFade(percent, startColor, endColor));

            await SendCommand(command, new List<string> {id.ToString()});
        }

        /*
         * Fades for a given timeSpan from start to end color with a specific bulb
         * @param timeSpan: timespan for fading         
         * @param startColor: start color in hex format
         * @param endColor: end color in hex format
         * @param id: id of specific bulb
         */
        public async Task Fade(TimeSpan timeSpan, string startColor, string endColor, int id)
        {
            var command = new LightCommand();

            command.SetColor(startColor);

            await SendCommand(command, new List<string> { id.ToString() });

            command = new LightCommand();

            command.SetColor(endColor);
            command.TransitionTime = timeSpan;

            await SendCommand(command, new List<string> { id.ToString() });
        }

        /*
         * Blink with a specific id
         */
        public async Task Blink(int id)
        {
            var command = new LightCommand();
            command.Alert = Alert.Once;

            await SendCommand(command, new List<string> { id.ToString() });
        }

        /*
         * Blink with all bulbs
         */
        public async Task Blink()
        {
            var command = new LightCommand();
            command.Alert = Alert.Once;

            await SendCommand(command);
        }

        /*
         * Send a LightCommand for specific bulbs to the hardware
         * @param command: LightCommand
         * @param deviceList: String list with all ids that will receive the given command
         */ 
        private async Task SendCommand(LightCommand command, List<string> deviceList )
        {
            await client.SendCommandAsync(command, deviceList);
        }

        /*
         * Send a LightCommand for all bulbs
         * @param command: LightCommand
         */ 
        private async Task SendCommand(LightCommand command)
        {
            await client.SendCommandAsync(command);
        }
        
    }
}
