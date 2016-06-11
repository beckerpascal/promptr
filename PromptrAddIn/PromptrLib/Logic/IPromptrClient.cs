using System;

namespace PromptrLib.Logic
{
    /*
     * Simple interface to have a basic API to connect external applications to the Promptr library
     */ 
    public interface IPromptrClient
    {
        /*
         * Starts the countdown with a total duration and/or a duration for each slide
         * @param totalDuration: duration for the whole presentation
         * @param slideDurations: duration array for each slide
         */
        void StartCountdown(TimeSpan totalDuration, TimeSpan[] slideDurations);

        /*
         * Ends count down
         */
        void EndCountdown();

        /*
         * Sets the current slide number to a given number
         * @param number: slide number
         */
        void SetCurrentSlideNumber(int number);
    }
}
