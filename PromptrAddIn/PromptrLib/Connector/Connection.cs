using System.Collections.Generic;
using System.Threading.Tasks;
using PromptrLib.Connector;
using Q42.HueApi;
using Q42.HueApi.Interfaces;

namespace PromptrLib
{
    public class Connection
    {
        private ILocalHueClient client;

        public Connection(ILocalHueClient client)
        {
            this.client = client;
        }

        public async Task TurnOn()
        {
            var command = new LightCommand();
            command.TurnOn();

            await SendCommand(command);
        }

        public async Task TurnOn(int id)
        {
            var command = new LightCommand();
            command.TurnOn();

            await SendCommand(command, new List<string> { id.ToString() });
        }

        public async Task TurnOff()
        {
            var command = new LightCommand();
            command.TurnOff();

            await SendCommand(command);
        }

        public async Task TurnOff(int id)
        {
            var command = new LightCommand();
            command.TurnOff();

            await SendCommand(command, new List<string> { id.ToString() });
        }

        public async Task Fade(int percent, string startColor, string endColor, int id)
        {
            ColorCalculation colorCalc = new ColorCalculation();
            var command = new LightCommand();

            command.SetColor(colorCalc.CalculateColorFade(percent, startColor, endColor));

            await SendCommand(command, new List<string> {id.ToString()});
        }



        private async Task SendCommand(LightCommand command, List<string> deviceList )
        {
            await client.SendCommandAsync(command, deviceList);
        }

        private async Task SendCommand(LightCommand command)
        {
            await client.SendCommandAsync(command);
        }
    }
}
