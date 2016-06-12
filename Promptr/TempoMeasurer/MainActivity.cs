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

namespace TempoMeasurer
{
    [Activity(Label = "TempoMeasurer", MainLauncher = true, Icon = "@drawable/icon")]
    public class MainActivity : Activity
    {
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

            // Get our button from the layout resource,
            // and attach an event to it
            Button button = FindViewById<Button>(Resource.Id.MyButton);

            _displayText = FindViewById<TextView>(Resource.Id.text);
            lastResult = new List<string>();

            //var client = new PromptrClient();
            //client.StartCountdown(new TimeSpan(0, 3, 0), new TimeSpan[0]);
            var client = new ConnectorClient();
            client.TurnOn();

            button.Click += delegate {
                button.Text = string.Format("{0} clicks!", count++);
                BeginProcessing();
                client.Blink();
           //     client.SetSpeechTempo(new Random().Next());
            };

            if (ActivityCompat.CheckSelfPermission(this, Manifest.Permission.RecordAudio) == Permission.Granted)
            {
                BeginProcessing();
            } else
            {
                ActivityCompat.RequestPermissions(this, new string[] { Manifest.Permission.RecordAudio }, 312);
            }
            
            
                        
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
            ProcessStrings(results);
        }

        private void ProcessStrings(IList<string> strings)
        {
            DateTime now = DateTime.UtcNow;

            double duration = now.Subtract(previousResultTimestamp).TotalSeconds;

            if (duration == 0) duration = .01;

            string result = "Results: ";
            int wordlength = 0;

            int i = 0;
            while (i < strings.Count && i < lastResult.Count && lastResult[i].Equals(strings[i]))
            {
                i++;
            }
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

            averageWordLengthPerSecond = alpha * averageWordLength + (1 - alpha) * averageWordLengthPerSecond;

            _displayText.Text = averageWordLengthPerSecond + "";

            if (averageWordLengthPerSecond < slowThreshold)
            {

            } else if (averageWordLengthPerSecond > fastThreshold)
            {

            } else
            {

            }
        }
    }
}

