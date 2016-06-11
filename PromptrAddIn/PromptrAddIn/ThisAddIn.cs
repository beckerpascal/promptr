using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Windows.Forms;
using System.Xml.Linq;
using PowerPoint = Microsoft.Office.Interop.PowerPoint;
using Office = Microsoft.Office.Core;
using PromptrLib;
using PromptrLib.Logic;

namespace PromptrAddIn
{
    public partial class ThisAddIn
    {

        IPromptrClient client;
        private void ThisAddIn_Startup(object sender, System.EventArgs e)
        {
            Globals.ThisAddIn.Application.SlideShowBegin += ShowActivePresentation;
            Globals.ThisAddIn.Application.SlideShowNextSlide += Application_SlideShowNextSlide;

            client = new PromptrClient();
            
        }

        private void Application_SlideShowNextSlide(PowerPoint.SlideShowWindow Wn)
        {
            client.SetCurrentSlideNumber(Wn.Presentation.SlideShowWindow.View.CurrentShowPosition);
        }

        private void ShowActivePresentation(PowerPoint.SlideShowWindow wn)
        {
            
            var ribbon = Globals.Ribbons.GetRibbon<MyRibbon>();
            
            client.StartCountdown(ribbon.TotalDuration, ribbon.SlideDurations);
        }

        private void ThisAddIn_Shutdown(object sender, System.EventArgs e)
        {
            client.EndCountdown();
        }

        #region VSTO generated code

        /// <summary>
        /// Required method for Designer support - do not modify
        /// the contents of this method with the code editor.
        /// </summary>
        private void InternalStartup()
        {
            this.Startup += new System.EventHandler(ThisAddIn_Startup);
            this.Shutdown += new System.EventHandler(ThisAddIn_Shutdown);
        }
        
        #endregion
    }
}
