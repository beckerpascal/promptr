using System.Runtime.CompilerServices;
using System.Threading;
using System.Threading.Tasks;
using Microsoft.VisualStudio.TestTools.UnitTesting;
using PromptrLib;
using PromptrLib.Connector;

namespace PromptrLibTests
{
    [TestClass]
    public class ConnectionTests
    {
        private readonly ConnectionFactory connFactory = new ConnectionFactory(Constants.APPKEY);
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
                await _target.Fade(i, Constants.COLOR_GREEN, Constants.COLOR_RED, 1);
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
