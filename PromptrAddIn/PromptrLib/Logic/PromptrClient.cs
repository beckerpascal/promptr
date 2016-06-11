using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Timers;

namespace PromptrLib.Logic
{
    public class PromptrClient : IPromptrClient
    {
        private IConnectorClient connection;

        private TimeSpan TotalDuration;

        private TimeSpan[] SlideDurations;

        private Timer timer;

        private int percentCounter;

        private int currentBulb;

        public PromptrClient()
        {
            connection = new ConnectorClient();
            timer = new Timer();
        }

        public void EndCountdown()
        {
            connection.TurnOff();
        }

        public void SetCurrentSlideNumber(int number)
        {
            connection.Blink(1, currentBulb);
        }

        public void StartCountdown(TimeSpan totalDuration, TimeSpan[] slideDurations)
        {
            TotalDuration = totalDuration;
            SlideDurations = slideDurations;

            timer = new Timer(TotalDuration.TotalMilliseconds / 300);
            timer.AutoReset = true;
            timer.Elapsed += (sender, args) =>
            {
                percentCounter++;
                connection.Fade(percentCounter, "00FF00", "FF0000");

                if (percentCounter == 100)
                {
                    percentCounter = 0;
                    currentBulb++;
                }
            };
            timer.Start();

            connection.Fade(100, "00FF00", "00FF00");
        }
    }
}
