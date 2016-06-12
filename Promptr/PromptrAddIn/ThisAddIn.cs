using PowerPoint = Microsoft.Office.Interop.PowerPoint;
using PromptrLib.Logic;

namespace PromptrAddIn
{
    public partial class ThisAddIn
    {

        IPromptrClient client;
        private bool started;

        private void ThisAddIn_Startup(object sender, System.EventArgs e)
        {
            Globals.ThisAddIn.Application.SlideShowBegin += ShowActivePresentation;
            Globals.ThisAddIn.Application.SlideShowEnd += (args) =>
            {
                started = false;
                client.EndCountdown();
            };
            Globals.ThisAddIn.Application.SlideShowNextSlide += Application_SlideShowNextSlide;

            client = new PromptrClient();
            
        }

        private void Application_SlideShowNextSlide(PowerPoint.SlideShowWindow Wn)
        {
            if (started) client.SetCurrentSlideNumber(Wn.Presentation.SlideShowWindow.View.CurrentShowPosition);
        }

        private void ShowActivePresentation(PowerPoint.SlideShowWindow wn)
        {
            started = true;

            var ribbon = Globals.Ribbons.GetRibbon<MyRibbon>();
            
            client.StartCountdown(ribbon.TotalDuration, ribbon.SlideDurations);
        }

        private void ThisAddIn_Shutdown(object sender, System.EventArgs e)
        {
            started = false;
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
