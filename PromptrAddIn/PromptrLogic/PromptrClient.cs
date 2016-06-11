using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using PromptrLib;
using Q42.HueApi;

namespace PromptrLogic
{
    public class PromptrClient : IPromptrClient
    {
        private Connection connection;

        public PromptrClient()
        {
            connection = new ConnectionFactory("x6gkNkXAp7Gv5yatJEXFahEE8oYkpe6SjKFOZKI8").GetConnection();
        }

        public async void TurnOn()
        {
            var command = new LightCommand();
            command.TurnOn();

            await connection.SendCommand(command);
        }

        public async void TurnOn(int id)
        {
            var command = new LightCommand();
            command.TurnOn();

            await connection.SendCommand(command, new List<string> { id.ToString() });
        }

        public async void TurnOff()
        {
            var command = new LightCommand();
            command.TurnOff();

            await connection.SendCommand(command);
        }

        public async void TurnOff(int id)
        {
            var command = new LightCommand();
            command.TurnOff();

            await connection.SendCommand(command, new List<string> { id.ToString() });
        }

    }
}
