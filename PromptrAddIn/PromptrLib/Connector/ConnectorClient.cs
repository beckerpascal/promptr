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

        public async void TurnOn(string color)
        {
            await connection.TurnOn(color);
        }


        public async void TurnOn(int id)
        {
            await connection.TurnOn(id);
        }

        public async void TurnOn(string color, int id)
        {
            await connection.TurnOn(color, id);
        }

        public async void TurnOff()
        {
            await connection.TurnOff();
        }

        public async void TurnOff(int id)
        {
            await connection.TurnOff(id);
        }

        public async void Pulsate(int hertz, int id)
        {
            await connection.Puls(id);
        }

        public async void Blink(int repeatCount, int id)
        {
            for (int blinkCount = 0; blinkCount < repeatCount; blinkCount++)
            {
                await connection.Blink(id);
            }
        }

        public async void Fade(int percent, string startColor, string endColor, int id)
        {
            await connection.Fade(percent, startColor, endColor, id);
        }

        public async void Fade(TimeSpan timeSpan, string startColor, string endColor, int id)
        {
            await connection.Fade(timeSpan, startColor, endColor, id);
        }
    }
}
