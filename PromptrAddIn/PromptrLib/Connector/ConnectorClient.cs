using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using PromptrLib;
using Q42.HueApi;

namespace PromptrLib
{
    public class ConnectorClient : IConnectorClient
    {
        private Connection connection;

        public ConnectorClient()
        {
            connection = new ConnectionFactory("x6gkNkXAp7Gv5yatJEXFahEE8oYkpe6SjKFOZKI8").GetConnection();
        }

        public async void TurnOn()
        {
            await connection.TurnOn();
        }

        public async void TurnOn(int id)
        {
            await connection.TurnOn(id);
        }

        public async void TurnOff()
        {
            await connection.TurnOff();
        }

        public async void TurnOff(int id)
        {
            await connection.TurnOff(id);
        }

        public void Pulsate(int hertz, int id)
        {
            throw new NotImplementedException();
        }

        public void Blink(int repeatCount, int id)
        {
            throw new NotImplementedException();
        }

        public async void Fade(int percent, string startColor, string endColor, int id)
        {
            await connection.Fade(percent, startColor, endColor, id);
        }
    }
}
