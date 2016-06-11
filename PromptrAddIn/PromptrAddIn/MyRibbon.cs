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
        private void MyRibbon_Load(object sender, RibbonUIEventArgs e)
        {
            Globals.ThisAddIn.Application.AfterPresentationOpen += (args) => { UpdateSlideButtons(); };
            Globals.ThisAddIn.Application.PresentationNewSlide += Application_PresentationNewSlide;
            button1.Click += (s, args) =>
            {
                UpdateSlideButtons();
            };

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

            dropDown1.Items.Clear();

            for (int i = 0; i < count; i++)
            {
                int number = i + 1;
                var item = Factory.CreateRibbonDropDownItem();
                item.Label = presentation.Slides[number].Name;
                dropDown1.Items.Add(item);
            }
        }

        private void editBox1_TextChanged(object sender, RibbonControlEventArgs e)
        {

        }

        private void editBox2_TextChanged(object sender, RibbonControlEventArgs e)
        {
            
        }

        private void dropDown1_SelectionChanged(object sender, RibbonControlEventArgs e)
        {

        }
    }
}
