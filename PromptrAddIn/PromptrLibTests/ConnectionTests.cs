using System.Threading;
using System.Threading.Tasks;
using Microsoft.VisualStudio.TestTools.UnitTesting;
using PromptrLib;
using Q42.HueApi;

namespace PromptrLibTests
{
    [TestClass]
    public class ConnectionTests
    {
        private ConnectionFactory connFactory;
        private Connection target;

        [TestInitialize]
        public void TestInitialize()
        {
            ConnectionFactory connFactory = new ConnectionFactory("x6gkNkXAp7Gv5yatJEXFahEE8oYkpe6SjKFOZKI8");
            target = connFactory.GetConnection();
        }

        [TestMethod]
        public async Task StartConnection()
        {
            var command = new LightCommand();
            command.TurnOn();
            command.SetColor("FFAA00");

            await target.SendCommand(command);

            Thread.Sleep(3000);

            command = new LightCommand();
            command.TurnOff();

            await target.SendCommand(command);
        }
    }
}
