using System.Collections.Generic;
using System.Threading.Tasks;
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

        public async Task SendCommand(LightCommand command, List<string> deviceList )
        {
            await client.SendCommandAsync(command, deviceList);
        }

        public async Task SendCommand(LightCommand command)
        {
            await client.SendCommandAsync(command);
        }
    }
}
