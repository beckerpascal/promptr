using System;
using System.Collections.Generic;
using System.Drawing;
using System.Linq;
using System.Text;
using Microsoft.Office.Tools.Ribbon;

namespace PromptrAddIn
{
    public partial class MyRibbon
    {
        public TimeSpan TotalDuration { get; private set; }
        public TimeSpan[] SlideDurations { get; private set; }

        private List<int> overwritten;

        private int currentSlideIndex;

        private void MyRibbon_Load(object sender, RibbonUIEventArgs e)
        {
            Globals.ThisAddIn.Application.AfterPresentationOpen += (args) => { UpdateSlideButtons(); };
            Globals.ThisAddIn.Application.PresentationNewSlide += Application_PresentationNewSlide;
            /*
            refreshButton.Click += (s, args) =>
            {
                UpdateSlideButtons();
            };
            */
            overwritten = new List<int>();

            TotalDurationDropDown.Items.Clear();

            for (int i = 0; i <= 60; i++)
            {
                var item = Factory.CreateRibbonDropDownItem();
                item.Label = i + "";
                TotalDurationDropDown.Items.Add(item);
            }

            TotalDurationDropDown.SelectedItemIndex = 3;

            //slideMinuteDropDown.Items.Clear();

            for (int i = 0; i <= 20; i++)
            {
                var item = Factory.CreateRibbonDropDownItem();
                item.Label = i + "";
                //slideMinuteDropDown.Items.Add(item);
            }

            //slideSecondsDropDown.Items.Clear();

            for (int i = 0; i <= 60; i++)
            {
                var item = Factory.CreateRibbonDropDownItem();
                item.Label = i + "";
                //slideSecondsDropDown.Items.Add(item);
            }

            TotalDuration = new TimeSpan(0, 3, 0);
            SlideDurations = new TimeSpan[0];


        }

        private void Application_PresentationNewSlide(Microsoft.Office.Interop.PowerPoint.Slide Sld)
        {
            UpdateSlideButtons();
        }

        private void UpdateSlideButtons()
        {
            var presentation = Globals.ThisAddIn.Application.ActivePresentation;

            if (presentation == null) return;

            int count = presentation.Slides.Count;

            //slidesDropDown.Items.Clear();

            for (int i = 0; i < count; i++)
            {
                int number = i + 1;
                var item = Factory.CreateRibbonDropDownItem();
                item.Label = presentation.Slides[number].Name;
                //slidesDropDown.Items.Add(item);
            }

            var oldDurations = SlideDurations;

            SlideDurations = new TimeSpan[count];

            int defaultSeconds = (int)(TotalDuration.TotalSeconds / count);

            for (int i = 0; i < oldDurations.Length; i++)
            {
                if (i < count)
                {
                    if (overwritten.Contains(i))
                    {
                        SlideDurations[i] = oldDurations[i];
                    } else
                    {
                        SlideDurations[i] = new TimeSpan(0, 0, defaultSeconds);
                    }
                    
                }
            }

            if (oldDurations.Length < count)
            {
                
                for (int i = oldDurations.Length; i < count; i++)
                {
                    SlideDurations[i] = new TimeSpan(0, 0, defaultSeconds);
                }
            }
        }
        /*
        private void slideSecondsDropDown_SelectionChanged(object sender, RibbonControlEventArgs e)
        {
            int minutes = SlideDurations[currentSlideIndex].Minutes;
            int seconds = 0;
            Int32.TryParse(slideSecondsDropDown.SelectedItem.Label, out seconds);
            SlideDurations[currentSlideIndex] = new TimeSpan(0, minutes, seconds);  
            
            if (!overwritten.Contains(currentSlideIndex))
            {
                overwritten.Add(currentSlideIndex);
            }             
        }

        private void slidesDropDown_SelectionChanged(object sender, RibbonControlEventArgs e)
        {
            currentSlideIndex = slidesDropDown.SelectedItemIndex;
            slideMinuteDropDown.SelectedItemIndex = SlideDurations[currentSlideIndex].Minutes;
            slideSecondsDropDown.SelectedItemIndex = SlideDurations[currentSlideIndex].Seconds;
        }
        

        private void slideMinuteDropDown_SelectionChanged(object sender, RibbonControlEventArgs e)
        {
            int seconds = SlideDurations[currentSlideIndex].Seconds;
            int minutes = 0;
            Int32.TryParse(slideMinuteDropDown.SelectedItem.Label, out minutes);
            SlideDurations[currentSlideIndex] = new TimeSpan(0, minutes, seconds);

            if (!overwritten.Contains(currentSlideIndex))
            {
                overwritten.Add(currentSlideIndex);
            }
        }
        */


        private void TotalDurationDropDown_SelectionChanged(object sender, RibbonControlEventArgs e)
        {
            int minutes = 0;
            Int32.TryParse(TotalDurationDropDown.SelectedItem.Label, out minutes);
            TotalDuration = new TimeSpan(0, minutes, 0);
        }


    }
}
