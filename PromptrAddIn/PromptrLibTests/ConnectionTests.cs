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
        private readonly ConnectionFactory connFactory = new ConnectionFactory("x6gkNkXAp7Gv5yatJEXFahEE8oYkpe6SjKFOZKI8");
        private Connection _target;

        [TestInitialize]
        public async void Initialize()
        {
            _target = connFactory.GetConnection();
            await _target.TurnOn();
        }
        
        [TestMethod]
        public async Task Fade()
        {
            for (int i = 0; i < 100; i++)
            {
                await _target.Fade(i, "#00FF00", "#FF0000", 1);
                Thread.Sleep(30);
            }
        }

        [TestMethod]
        public async Task Blink()
        {
            await _target.Blink(1);
        }
        
        [TestMethod]
        public async Task BlinkAll()
        {
            await _target.Blink();
        }

        [TestCleanup]
        public async void Cleanup()
        {
            await _target.TurnOff();
        }
        
    }
}
