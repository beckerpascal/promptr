using System;
using Microsoft.VisualStudio.TestTools.UnitTesting;

namespace PromptrLibTests
{
    [TestClass]
    public class ConnectorTests
    {
        [TestMethod]
        public void ColorCalculator()
        {
            PromptrLib.Connector.ColorCalculation colorCalculatorA 
                = new PromptrLib.Connector.ColorCalculation();
            var a = colorCalculatorA.CalculateColorFade(55, "#FFAA66", "#FF00FF");
        }
    }
}
