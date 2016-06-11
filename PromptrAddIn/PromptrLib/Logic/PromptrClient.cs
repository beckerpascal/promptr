﻿using System;
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

        private Timer slideTimer;

        private int currentBulb;

        private int currentSlide;

        public PromptrClient()
        {
            connection = new ConnectorClient();
            timer = new Timer();
            currentBulb = 0;
        }

        public void EndCountdown()
        {
            connection.TurnOff();
        }

        public void SetCurrentSlideNumber(int number)
        {
            //if (slideTimer != null)
            //{
            //    slideTimer.Stop();
            //}

            //slideTimer = new Timer(SlideDurations[number - 1].TotalMilliseconds);
            //slideTimer.AutoReset = false;
            //slideTimer.Elapsed += (sender, args) =>
            //{
            //    connection.Blink(1, currentBulb);
            //};
            //slideTimer.Start();
        }

        public void StartCountdown(TimeSpan totalDuration, TimeSpan[] slideDurations)
        {
            TotalDuration = totalDuration;
            SlideDurations = slideDurations;
            currentBulb = 0;

            connection.TurnOn("#60854E");

            timer = new Timer(TotalDuration.TotalMilliseconds / 3);
            timer.AutoReset = true;
            timer.Elapsed += (sender, args) =>
            {
                FadeCurrentLight();
                if (currentBulb > 3)
                {
                    timer.Stop();
                }
            };

            FadeCurrentLight();

            timer.Start();

            Timer endTimer = new Timer(TotalDuration.TotalMilliseconds - 30000);
            endTimer.Elapsed += (sender, args) =>
            {
                for (int i = 1; i < 3; i++)
                {
                    connection.Blink(3, i);
                }
                
            };
            endTimer.Start();

        }

        private void FadeCurrentLight()
        {
            currentBulb++;
            connection.Fade(new TimeSpan(0, 0, (int)TotalDuration.TotalSeconds / 3), "#60854E", "#FF0000", currentBulb);
            
        }
    }
}
