using System;
using System.Timers;

namespace PromptrLib.Logic
{
    public class PromptrClient : IPromptrClient
    {
        // Private member for handling the connection for the whole lifetime
        private IConnectorClient _connection;

        // Private members for timers and timespans
        private TimeSpan _totalDuration;
        private TimeSpan[] _slideDurations;
        private Timer _timer;
        private Timer _slideTimer;

        // Private members for handling bulbs and slides
        private int _currentBulb;
        private int _currentSlide;

        /*
         * Constructor for creating a new PromptrClient with connection, timer and starting with bulb 0
         */ 
        public PromptrClient()
        {
            _connection = new ConnectorClient();
            _timer = new Timer();
            _currentBulb = 0;
        }

        /*
         * Ends count down
         */ 
        public void EndCountdown()
        {
            _connection.TurnOff();
        }

        /*
         * Sets the current slide number to a given number
         * @param number: slide number
         */ 
        public void SetCurrentSlideNumber(int number)
        {
            //if (_slideTimer != null)
            //{
            //    _slideTimer.Stop();
            //}

            //_slideTimer = new Timer(_slideDurations[number - 1].TotalMilliseconds);
            //_slideTimer.AutoReset = false;
            //_slideTimer.Elapsed += (sender, args) =>
            //{
            //   _connection.Blink(1, _currentBulb);
            //};
            //_slideTimer.Start();
        }

        /*
         * Starts the countdown with a total duration and/or a duration for each slide
         * @param totalDuration: duration for the whole presentation
         * @param slideDurations: duration array for each slide
         */ 
        public void StartCountdown(TimeSpan totalDuration, TimeSpan[] slideDurations)
        {
            _totalDuration = totalDuration;
            _slideDurations = slideDurations;
            _currentBulb = 0;

            _connection.TurnOn(Constants.COLOR_LIME);

            _timer = new Timer(_totalDuration.TotalMilliseconds / Constants.AMOUNT_OF_BULBS);
            _timer.AutoReset = true;
            _timer.Elapsed += (sender, args) =>
            {
                FadeCurrentLight();
                if (_currentBulb > Constants.AMOUNT_OF_BULBS)
                {
                    _timer.Stop();
                }
            };

            FadeCurrentLight();

            _timer.Start();

            Timer endTimer = new Timer(_totalDuration.TotalMilliseconds - 30000);
            endTimer.Elapsed += (sender, args) =>
            {
                _connection.Blink();
                
            };
            endTimer.Start();

        }

        /*
         * Fades the current light starting with the current bulb from start to end color in the specified time
         */ 
        private void FadeCurrentLight()
        {
            _currentBulb++;
            _connection.Fade(new TimeSpan(0, 0, (int)_totalDuration.TotalSeconds / Constants.AMOUNT_OF_BULBS), Constants.COLOR_LIME, Constants.COLOR_RED, _currentBulb);
            
        }
    }
}
