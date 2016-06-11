using System.Drawing;

namespace PromptrLib.Connector
{
    public class ColorCalculation
    {
        public string CalculateColorFade(int percent, string startColor, string endColor)
        {
            Color source = (Color) ColorTranslator.FromHtml(startColor);
            Color target = (Color) ColorTranslator.FromHtml(endColor);



            var r = (byte) (source.R + (target.R - source.R)*percent/100.0);
            var g = (byte) (source.G + (target.G - source.G)*percent/100.0);
            var b = (byte) (source.B + (target.B - source.B)*percent/100.0);



            return ColorTranslator.ToHtml(Color.FromArgb(255, r, g, b));
        }
    }
}
