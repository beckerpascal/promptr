using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace PromptrLib.Logic
{
    public interface IPromptrClient
    {
        void StartCountdown(TimeSpan totalDuration, TimeSpan[] slideDurations);

        void EndCountdown();

        void SetCurrentSlideNumber(int number);

        void SetSpeechTempo(int tempoLevel);
    }
}
