using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

using Android.App;
using Android.Content;
using Android.OS;
using Android.Runtime;
using Android.Views;
using Android.Widget;
using Java.Lang;
using Android.Media;

namespace TempoMeasurer
{
    public class AudioThread : Thread
    {
        private bool stopped = false;

        public AudioThread()
        {
            Start();
        }

        public override void Run()
        {
            Android.OS.Process.SetThreadPriority(ThreadPriority.UrgentAudio);
            AudioRecord recorder = null;
            short[][] buffers = new short[256][];
            int ix = 0;

            try
            { // ... initialise

                int N = AudioRecord.GetMinBufferSize(8000, ChannelIn.Mono, Android.Media.Encoding.Pcm16bit);

                recorder = new AudioRecord(AudioSource.Mic,
                                           8000,
                                           ChannelIn.Mono,
                                           Android.Media.Encoding.Pcm16bit,
                                           N * 10);

                recorder.StartRecording();

                // ... loop

                while (!stopped)
                {
                    short[] buffer = buffers[ix++ % buffers.Length];

                    N = recorder.Read(buffer, 0, buffer.Length);
                    //process is what you will do with the data...not defined here
                    process(buffer);
                }
            }
            catch (Throwable x)
            {
                //
            }
            finally
            {
                close();
            }
        }

        private void close()
        {
            stopped = true;
        }

        private void process(short[] buffer)
        {
            Console.WriteLine(DateTime.UtcNow.Millisecond);
        }
    }
}