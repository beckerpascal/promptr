namespace PromptrAddIn
{
    partial class MyRibbon : Microsoft.Office.Tools.Ribbon.RibbonBase
    {
        /// <summary>
        /// Required designer variable.
        /// </summary>
        private System.ComponentModel.IContainer components = null;

        public MyRibbon()
            : base(Globals.Factory.GetRibbonFactory())
        {
            InitializeComponent();
        }

        /// <summary> 
        /// Clean up any resources being used.
        /// </summary>
        /// <param name="disposing">true if managed resources should be disposed; otherwise, false.</param>
        protected override void Dispose(bool disposing)
        {
            if (disposing && (components != null))
            {
                components.Dispose();
            }
            base.Dispose(disposing);
        }

        #region Component Designer generated code

        /// <summary>
        /// Required method for Designer support - do not modify
        /// the contents of this method with the code editor.
        /// </summary>
        private void InitializeComponent()
        {
            this.tab1 = this.Factory.CreateRibbonTab();
            this.Duration = this.Factory.CreateRibbonGroup();
            this.TotalDurationDropDown = this.Factory.CreateRibbonDropDown();
            this.tab1.SuspendLayout();
            this.Duration.SuspendLayout();
            this.SuspendLayout();
            // 
            // tab1
            // 
            this.tab1.Groups.Add(this.Duration);
            this.tab1.Label = "Promptr";
            this.tab1.Name = "tab1";
            // 
            // Duration
            // 
            this.Duration.Items.Add(this.TotalDurationDropDown);
            this.Duration.Label = "Presentation Duration";
            this.Duration.Name = "Duration";
            // 
            // TotalDurationDropDown
            // 
            this.TotalDurationDropDown.Label = "Minutes";
            this.TotalDurationDropDown.Name = "TotalDurationDropDown";
            this.TotalDurationDropDown.SelectionChanged += new Microsoft.Office.Tools.Ribbon.RibbonControlEventHandler(this.TotalDurationDropDown_SelectionChanged);
            // 
            // MyRibbon
            // 
            this.Name = "MyRibbon";
            this.RibbonType = "Microsoft.PowerPoint.Presentation";
            this.Tabs.Add(this.tab1);
            this.Load += new Microsoft.Office.Tools.Ribbon.RibbonUIEventHandler(this.MyRibbon_Load);
            this.tab1.ResumeLayout(false);
            this.tab1.PerformLayout();
            this.Duration.ResumeLayout(false);
            this.Duration.PerformLayout();
            this.ResumeLayout(false);

        }

        #endregion

        internal Microsoft.Office.Tools.Ribbon.RibbonTab tab1;
        internal Microsoft.Office.Tools.Ribbon.RibbonGroup Duration;
        internal Microsoft.Office.Tools.Ribbon.RibbonDropDown TotalDurationDropDown;
    }

    partial class ThisRibbonCollection
    {
        internal MyRibbon MyRibbon
        {
            get { return this.GetRibbon<MyRibbon>(); }
        }
    }
}
