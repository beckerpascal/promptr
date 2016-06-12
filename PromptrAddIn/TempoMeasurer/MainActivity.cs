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

namespace TempoMeasurer
{
    [Activity(Label = "TempoMeasurer", MainLauncher = true, Icon = "@drawable/icon")]
    public class MainActivity : Activity
    {
        int count = 1;

        protected override void OnCreate(Bundle bundle)
        {
            base.OnCreate(bundle);

            // Set our view from the "main" layout resource
            SetContentView(Resource.Layout.Main);

            // Get our button from the layout resource,
            // and attach an event to it
            Button button = FindViewById<Button>(Resource.Id.MyButton);

            var client = new PromptrClient();
            client.StartCountdown(new TimeSpan(0, 3, 0), new TimeSpan[0]);

            button.Click += delegate {
                button.Text = string.Format("{0} clicks!", count++);
                client.SetSpeechTempo(new Random().Next());
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
        }

        private void Recognizer_Results(object sender, ResultsEventArgs e)
        {
            IList<string> results = e.Results.GetStringArrayList(SpeechRecognizer.ResultsRecognition);
            string result = "Results: ";
            foreach (string s in results)
            {
                result += s + " ";
            }
            Console.WriteLine()
        }

        private void Recognizer_PartialResults(object sender, PartialResultsEventArgs e)
        {
            IList<string> results = e.PartialResults.GetStringArrayList(SpeechRecognizer.ResultsRecognition);
        }
    }
}

