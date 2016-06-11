using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace PromptrLib
{
    public interface IConnectorClient
    {
        void TurnOn();
        void TurnOn(int id);
        void TurnOff();
        void TurnOff(int id);
        void Pulsate(int hertz, int id);
        void Blink(int repeatCount, int id);
        void Fade(int percent, string startColor, string endColor, int id);
        void Fade(TimeSpan timeSpan, string startColor, string endColor, int id);
    }
}
