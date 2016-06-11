using System.Runtime.CompilerServices;
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
        public async void Initialize()
        {
            ConnectionFactory connFactory = new ConnectionFactory("x6gkNkXAp7Gv5yatJEXFahEE8oYkpe6SjKFOZKI8");
            target = connFactory.GetConnection();
            await target.TurnOn();
        }
        
        [TestMethod]
        public async Task Fade()
        {
            for (int i = 0; i < 100; i++)
            {
                await target.Fade(i, "#00FF00", "#FF0000", 1);
                Thread.Sleep(30);
            }
        }
        
        [TestCleanup]
        public async void Cleanup()
        {
            await target.TurnOff();
        }
        
    }
}
