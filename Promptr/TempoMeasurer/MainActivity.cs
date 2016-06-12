using System;
using Android.App;
using Android.Content;
using Android.Runtime;
using Android.Views;
using Android.Widget;
using Android.OS;
using Android;
using Android.Content.PM;
using Android.Support.V4.App;
using Android.Speech;
using System.Collections.Generic;
using TempoMeasurer.Connector;
using System.Timers;
using Java.Lang;

namespace TempoMeasurer
{
    [Activity(Label = "TempoMeasurer", MainLauncher = true, Icon = "@drawable/icon")]
    public class MainActivity : Activity
    {
        private Timer blinkTimer;
        private IConnectorClient client;

        private ImageView _turtle;
        private ImageView _rabbit;
        private ImageView _man;

        private View _animatedView;

        int count = 1;

        DateTime previousResultTimestamp;

        const double alpha = 0.1;

        TextView _displayText;

        IList<string> lastResult;

        double averageWordLengthPerSecond = 0;
        private double slowThreshold = 15;
        private double fastThreshold = 5;

        protected override void OnCreate(Bundle bundle)
        {
            base.OnCreate(bundle);

            // Set our view from the "main" layout resource
            SetContentView(Resource.Layout.Main);

            _turtle = FindViewById<ImageView>(Resource.Id.slowerButton);
            _rabbit = FindViewById<ImageView>(Resource.Id.fasterButton);
            _man = FindViewById<ImageView>(Resource.Id.normalButton);

            _turtle.Click += (sender, args) =>
            {
                ShowActive(_turtle);
                ShowInactive(_rabbit);
                ShowInactive(_man);
                BlinkLowSpeed();
            };

            _rabbit.Click += (sender, args) =>
            {
                ShowActive(_rabbit);
                ShowInactive(_turtle);
                ShowInactive(_man);
                BlinkHighSpeed();
            };

            _man.Click += (sender, args) =>
            {
                ShowPassive(_man);
                ShowInactive(_rabbit);
                ShowInactive(_turtle);
                StopBlinking();
            };

            try
            {
                client = new ConnectorClient();
            }
            catch (System.Exception e)
            {
                Console.WriteLine(e.Message);
            }

            lastResult = new List<string>();

            if (ActivityCompat.CheckSelfPermission(this, Manifest.Permission.RecordAudio) == Permission.Granted)
            {
                BeginProcessing();
            }
            else
            {
                ActivityCompat.RequestPermissions(this, new string[] { Manifest.Permission.RecordAudio }, 312);
            }



        }

        private void ShowPassive(ImageView view)
        {
            view.Alpha = 1f;
            _animatedView = null;
            view.Animate().ScaleX(1f).ScaleY(1f).Rotation(0).SetDuration(500).Start();
        }

        private void ShowActive(ImageView view)
        {
            _animatedView = view;
            view.Alpha = 1f;
            view.Animate().Rotation(15f).ScaleX(1.1f).ScaleY(1.1f).SetDuration(500).WithEndAction(RotateLeft()).Start();
        }

        private Runnable RotateLeft(bool grow = false)
        {
            return new Runnable(() =>
            {
                if (_animatedView == null) return;
                float scale = grow ? 1.1f : 1f;
                _animatedView.Animate().Rotation(-30f).ScaleX(scale).ScaleY(scale).SetDuration(500).WithEndAction(RotateRight(!grow)).Start();
            });
        }

        private Runnable RotateRight(bool grow = false)
        {
            return new Runnable(() =>
            {
                if (_animatedView == null) return;
                float scale = grow ? 1.1f : 1f;
                _animatedView.Animate().Rotation(15f).ScaleX(scale).ScaleY(scale).SetDuration(500).WithEndAction(RotateLeft(grow)).Start();
            });
        }

        private void ShowInactive(ImageView view)
        {
            view.Alpha = .85f;
            view.Animate().ScaleX(1f).ScaleY(1f).Rotation(0).SetDuration(500).Start();
        }

        public override void OnRequestPermissionsResult(int requestCode, string[] permissions, [GeneratedEnum] Permission[] grantResults)
        {
            if (requestCode == 312)
            {
                if (grantResults[0] == Permission.Granted)
                {
                      BeginProcessing();
                }
            }
        }

        private void BeginProcessing()
        {
            SpeechRecognizer recognizer = SpeechRecognizer.CreateSpeechRecognizer(this);
            recognizer.PartialResults += Recognizer_PartialResults;
            recognizer.Results += Recognizer_Results;
            recognizer.Error += (sender, args) =>
            {
                Console.WriteLine(args.Error.ToString());
            };

            previousResultTimestamp = DateTime.UtcNow;

            Intent recognizerIntent = new Intent(RecognizerIntent.ActionRecognizeSpeech);
            recognizerIntent.PutExtra(RecognizerIntent.ExtraPartialResults, true);
            recognizerIntent.PutExtra(RecognizerIntent.ExtraLanguage, "de-DE");
            recognizer.StartListening(recognizerIntent);

            new Handler(Looper.MyLooper()).PostDelayed(() =>
            {
                if (DateTime.UtcNow.Subtract(previousResultTimestamp).TotalSeconds > 3)
                {
                    BeginProcessing();
                }
            }, 3500);
        }

        private void Recognizer_Results(object sender, ResultsEventArgs e)
        {
            IList<string> results = e.Results.GetStringArrayList(SpeechRecognizer.ResultsRecognition);
            ProcessStrings(results);
            Console.WriteLine("Result received.");
            BeginProcessing();
        }

        private void Recognizer_PartialResults(object sender, PartialResultsEventArgs e)
        {
            IList<string> results = e.PartialResults.GetStringArrayList(SpeechRecognizer.ResultsRecognition);
            Console.WriteLine("Partial Result");
     //       ProcessStrings(results);
        }

        private void ProcessStrings(IList<string> strings)
        {
            if (strings.Count == 0 || (strings.Count == 1 && strings[0].Equals(""))) return;
            DateTime now = DateTime.UtcNow;

            double duration = now.Subtract(previousResultTimestamp).TotalSeconds;

            string longest = "";
            foreach (string s in strings)
            {
                if (s.Length > longest.Length) longest = s;
            }
            strings = new List<string>(longest.Split(' '));

            if (duration == 0) duration = .01;

            string result = "Results: ";
            int wordlength = 0;

            int i = 0;
            //while (i < strings.Count && i < lastResult.Count && lastResult[i].Equals(strings[i]))
            //{
            //    i++;
            //}
            //if (i < strings.Count && i < lastResult.Count)
            //{
            //    Console.WriteLine(lastResult[i] + " != " + strings[i]);
            //}
            
            result += i + " ";
            for (int j = i; j < strings.Count; j++)
            {
                string s = strings[j];
                wordlength += s.Length;
                result += s + " ";
            }
            Console.WriteLine(result);

            lastResult = strings;

            double averageWordLength = wordlength / duration;

            Console.WriteLine("Duration: " + duration + ", Letters: " + wordlength);

            averageWordLengthPerSecond = alpha * averageWordLength + (1 - alpha) * averageWordLengthPerSecond;

            _displayText.Text = averageWordLengthPerSecond + "";

            if (averageWordLengthPerSecond < slowThreshold)
            {

            }
            else if (averageWordLengthPerSecond > fastThreshold)
            {

            }
            else
            {

            }
        }

        private void BlinkHighSpeed()
        {
            StopBlinking();

            blinkTimer = new Timer(650);
            blinkTimer.AutoReset = true;
            blinkTimer.Elapsed += (sender, args) =>
            {
                client.Blink(2);
            };
            blinkTimer.Start();
        }

        private void StopBlinking()
        {
            if (blinkTimer != null)
            {
                blinkTimer.Stop();
            }
        }

        private void BlinkLowSpeed()
        {
            StopBlinking();

            blinkTimer = new Timer(2000);
            blinkTimer.AutoReset = true;
            blinkTimer.Elapsed += (sender, args) =>
            {
                client.Blink(2);
            };
            blinkTimer.Start();
        }
    }
}

