using System;

namespace PromptrConnector.Connector
{
    /*
     * Class for processing calculations with colors
     */ 
    public class ColorCalculation
    {
        /*
         * Calculates the color between two given colors with a percentage
         * @param percent: value between 0 and 100
         * @param startColor: start color
         * @param endColor: end color
         */ 
        public string CalculateColorFade(int percent, string startColor, string endColor)
        {
            var startR = Convert.ToInt32(startColor.Substring(1, 2), 16);
            var startG = Convert.ToInt32(startColor.Substring(3, 2), 16);
            var startB = Convert.ToInt32(startColor.Substring(5, 2), 16);

            var zielR = Convert.ToInt32(endColor.Substring(1, 2), 16);
            var zielG = Convert.ToInt32(endColor.Substring(3, 2), 16);
            var zielB = Convert.ToInt32(endColor.Substring(5, 2), 16);

            var resultA = (startR + (zielR - startR)*percent/100).ToString("X2");
            var resultG = (startG + (zielG - startG)*percent/100).ToString("X2");
            var resultB = (startB + (zielB - startB)*percent/100).ToString("X2");

            var result =    "#" +       
                            resultA +
                            resultG +
                            resultB;

            return result;
        }
    }
}
