using Java.Lang;
using System.Drawing;

namespace TempoMeasurer.Connector
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
            Android.Graphics.Color source = Android.Graphics.Color.ParseColor(startColor);
            Android.Graphics.Color target = Android.Graphics.Color.ParseColor(endColor);

            var r = (byte) (source.R + (target.R - source.R)*percent/100.0);
            var g = (byte) (source.G + (target.G - source.G)*percent/100.0);
            var b = (byte) (source.B + (target.B - source.B)*percent/100.0);

            Android.Graphics.Color color = new Android.Graphics.Color(r, g, b);

            return "#" + Integer.ToString(r, 16) + Integer.ToString(g, 16)
                + Integer.ToString(b, 16);
        }
    }
}
