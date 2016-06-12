using System;

namespace TempoMeasurer.Connector
{
    /*
     * Interface for handling all the hardware calls to the Hue Bridge 
     */ 
    public interface IConnectorClient
    {
        void TurnOn();
        void TurnOn(string color);
        void TurnOn(int id);
        void TurnOn(string color, int id);
        void TurnOff();
        void TurnOff(int id);
        void Blink(int id);
        void Blink();
        void Fade(int percent, string startColor, string endColor, int id);
        void Fade(TimeSpan timeSpan, string startColor, string endColor, int id);
    }
}
