using System;
using System.Collections.Generic;
using System.Linq;
using System.Threading.Tasks;
using System.Windows.Forms;
using System.Drawing;
using System.Runtime.InteropServices;

namespace CornEmpireDemo
{
    static class Program
    {
        [STAThread]
        public static void Main(string[] args)
        {
            Application.EnableVisualStyles();
            Application.SetCompatibleTextRenderingDefault(false);

            string ProgramDirectoryPath = System.IO.Path.GetDirectoryName(System.Reflection.Assembly.GetExecutingAssembly().Location);
            string Untitled1ImageFilePath = System.IO.Path.Combine(ProgramDirectoryPath, "Untitled1.png");
            string CornFieldImageFilePath = System.IO.Path.Combine(ProgramDirectoryPath, "CornField.png");
            string OilWellImageFilePath = System.IO.Path.Combine(ProgramDirectoryPath, "OilWell.png");
            string McDonaldsImageFilePath = System.IO.Path.Combine(ProgramDirectoryPath, "McDonalds.png");

            Game GameState = new Game();

            Form MainForm = new Form();
            MainForm.Size = new Size(640, 480);
            MainForm.Text = "Corn Empire Demo";



            Label DollarLabel = new Label();
            DollarLabel.Location = new Point(50, 300);
            DollarLabel.Size = new Size(100, 32);
            DollarLabel.Text = "Dollars:";

            Label CornLabel = new Label();
            CornLabel.Location = new Point(150, 300);
            CornLabel.Size = new Size(100, 32);
            CornLabel.Text = "Corn:";

            Label OilBarrelLabel = new Label();
            OilBarrelLabel.Location = new Point(250, 300);
            OilBarrelLabel.Size = new Size(100, 32);
            OilBarrelLabel.Text = "OilBarrels:";

            Label BurgerlLabel = new Label();
            BurgerlLabel.Location = new Point(350, 300);
            BurgerlLabel.Size = new Size(100, 32);
            BurgerlLabel.Text = "Burgers:";


            Button CornButton = new Button();
            CornButton.Location = new Point(25, 100);
            CornButton.Size = new Size(100, 100);
            CornButton.Text = "Get corn";
            CornButton.Click += (object sender, EventArgs e) =>
            {
                GameState.ResourceTable[GameState.GetResourceIndex(Game.ResourceTypeId.Corn)].Amount += 1;
            };

            Button BuyCornFieldButton = new Button();
            BuyCornFieldButton.Location = new Point(400, 50);
            BuyCornFieldButton.Size = new Size(100, 45);
            BuyCornFieldButton.Text = "Buy corn field";
            BuyCornFieldButton.Image = Image.FromFile(CornFieldImageFilePath);
            BuyCornFieldButton.Click += (object sender, EventArgs e) =>
            {
                GameState.BuyResource(Game.ResourceTypeId.CornField);
            };

            Button BuyOilWellButton = new Button();
            BuyOilWellButton.Location = new Point(400, 50 + 50);
            BuyOilWellButton.Size = new Size(100, 45);
            BuyOilWellButton.Text = "Buy oil well";
            BuyOilWellButton.Image = Image.FromFile(OilWellImageFilePath);
            BuyOilWellButton.Click += (object sender, EventArgs e) =>
            {
                GameState.BuyResource(Game.ResourceTypeId.OilWell);
            };

            Button BuyMcDonaldsButton = new Button();
            BuyMcDonaldsButton.Location = new Point(400, 50 + 50 + 50);
            BuyMcDonaldsButton.Size = new Size(100, 45);
            BuyMcDonaldsButton.Text = "Buy McDonalds";
            BuyMcDonaldsButton.Image = Image.FromFile(McDonaldsImageFilePath);
            BuyMcDonaldsButton.Click += (object sender, EventArgs e) =>
            {
                GameState.BuyResource(Game.ResourceTypeId.McDonalds);
            };



            Button SellFieldButton = new Button();
            SellFieldButton.Location = new Point(150, 332);
            SellFieldButton.Size = new Size(100, 32);
            SellFieldButton.Text = "Sell corn";
            SellFieldButton.Click += (object sender, EventArgs e) =>
            {
                GameState.SellResource(Game.ResourceTypeId.Corn);
            };

            Button SellOilButton = new Button();
            SellOilButton.Location = new Point(250, 332);
            SellOilButton.Size = new Size(100, 32);
            SellOilButton.Text = "Sell oil";
            SellOilButton.Click += (object sender, EventArgs e) =>
            {
                GameState.SellResource(Game.ResourceTypeId.OilBarrel);
            };

            Button SellBurgerButton = new Button();
            SellBurgerButton.Location = new Point(350, 332);
            SellBurgerButton.Size = new Size(100, 32);
            SellBurgerButton.Text = "Sell burger";
            SellBurgerButton.Click += (object sender, EventArgs e) =>
            {
                GameState.SellResource(Game.ResourceTypeId.Burger);
            };


            System.Windows.Forms.Timer MyTimer = new System.Windows.Forms.Timer();
            MyTimer.Interval = 100;
            MyTimer.Start();
            MyTimer.Tick += (object sender, EventArgs e) =>
            {
                GameState.Tick(0.1f);
                DollarLabel.Text = "Dollars:" + GameState.GetResourceAmount(Game.ResourceTypeId.Dollar).ToString();
                CornLabel.Text = "Corn:" + GameState.GetResourceAmount(Game.ResourceTypeId.Corn).ToString();
                OilBarrelLabel.Text = "OilBarrels:" + GameState.GetResourceAmount(Game.ResourceTypeId.OilBarrel).ToString();
                BurgerlLabel.Text = "Burgers:" + GameState.GetResourceAmount(Game.ResourceTypeId.Burger).ToString();
            };

            MainForm.Controls.Add(DollarLabel);
            MainForm.Controls.Add(CornLabel);
            MainForm.Controls.Add(OilBarrelLabel);
            MainForm.Controls.Add(BurgerlLabel);

            MainForm.Controls.Add(SellFieldButton);
            MainForm.Controls.Add(SellOilButton);
            MainForm.Controls.Add(SellBurgerButton);

            MainForm.Controls.Add(CornButton);
            MainForm.Controls.Add(BuyCornFieldButton);
            MainForm.Controls.Add(BuyOilWellButton);
            MainForm.Controls.Add(BuyMcDonaldsButton);



            Application.Run(MainForm);
        }


    }

    class Game
    {
        public enum ResourceTypeId
        {
            Undefined = 0, // Must be at index zero
            Dollar,
            Corn,
            OilBarrel,
            Burger,
            CornField,
            OilWell,
            McDonalds,
            ResourceTypeIdCount
        }

        [StructLayout(LayoutKind.Sequential)]
        public unsafe struct TestT
        {
            public fixed float a[(int)ResourceTypeId.ResourceTypeIdCount];

        }

        public struct ResourceGeneration
        {
            public ResourceTypeId Id;
            public UInt64 Value;
            public ResourceGeneration(ResourceTypeId IdParameter, UInt64 ValueParameter)
            {
                Id = IdParameter;
                Value = ValueParameter;
            }
        }

        public struct Resource
        {
            public ResourceTypeId Id;
            public string Name;
            public UInt64 Value;
            public UInt64 Amount;
            public ResourceGeneration[] GenerationTable;
            public Resource(ResourceTypeId IdParameter, string NameParameter, UInt64 ValueParameter, UInt64 AmountParameter, ResourceGeneration[] GenerationTableParameter)
            {
                Id = IdParameter;
                Name = NameParameter;
                Value = ValueParameter;
                Amount = AmountParameter;
                GenerationTable = GenerationTableParameter;
            }
        }

        public Resource[] ResourceTable;
        public float UnprocessedTime;

        public int GetResourceIndex(ResourceTypeId Id)
        {
            for (int n = ResourceTable.Length, i = 0; i < n; i++)
            {
                if (ResourceTable[i].Id == Id)
                {
                    return i;
                }
            }
            return 0; // index of Undefined
        }

        public UInt64 GetResourceAmount(ResourceTypeId Id)
        {
            int ResourceIndex = GetResourceIndex(Id);
            if (ResourceIndex == 0)
            {
                return 0; // index of Undefined
            }
            return ResourceTable[ResourceIndex].Amount;
        }

        public bool SellResource(ResourceTypeId ResourceType, UInt64 ResourceAmount = 1, bool OnlyCheckIfTransactionIsPossible = false)
        {
            int ResourceIndex = GetResourceIndex(ResourceType);
            if (ResourceIndex == 0)
            {
                return false; // Undefined resource
            }
            if (ResourceTable[ResourceIndex].Amount < ResourceAmount)
            {
                return false; // Not enough to sell
            }

            int DollarResourceIndex = GetResourceIndex(ResourceTypeId.Dollar);
            UInt64 DollarAmount = ResourceAmount * (ResourceTable[ResourceIndex].Value / ResourceTable[DollarResourceIndex].Value);
            if (OnlyCheckIfTransactionIsPossible == false)
            {
                ResourceTable[ResourceIndex].Amount -= ResourceAmount;
                ResourceTable[DollarResourceIndex].Amount += DollarAmount;
            }
            return true;
        }

        public bool BuyResource(ResourceTypeId ResourceType, UInt64 ResourceAmount = 1, bool OnlyCheckIfTransactionIsPossible = false)
        {
            int ResourceIndex = GetResourceIndex(ResourceType);
            if (ResourceIndex == 0)
            {
                return false; // Undefined resource
            }
            int DollarResourceIndex = GetResourceIndex(ResourceTypeId.Dollar);
            UInt64 DollarAmount = ResourceAmount * (ResourceTable[ResourceIndex].Value / ResourceTable[DollarResourceIndex].Value);
            if (ResourceTable[DollarResourceIndex].Amount < DollarAmount)
            {
                return false; // Not enough to buy
            }

            if (OnlyCheckIfTransactionIsPossible == false)
            {
                ResourceTable[ResourceIndex].Amount += ResourceAmount;
                ResourceTable[DollarResourceIndex].Amount -= DollarAmount;
            }
            return true;
        }

        public void Tick(float TimeDelta)
        {
            UnprocessedTime += TimeDelta;
            UInt64 TimeUnitCount = (UInt64)(UnprocessedTime / 1.0f);
            if (TimeUnitCount == 0)
            {
                return;
            }
            UnprocessedTime -= (float)TimeUnitCount;

            for (int ResourceCount = ResourceTable.Length, i = 0; i < ResourceCount; i++)
            {
                for (int GenerationCount = ResourceTable[i].GenerationTable.Length, j = 0; j < GenerationCount; j++)
                {
                    int GenerationResourceIndex = GetResourceIndex(ResourceTable[i].GenerationTable[j].Id);
                    if (GenerationResourceIndex != 0)
                    {
                        ResourceTable[GenerationResourceIndex].Amount += TimeUnitCount * ResourceTable[i].Amount * ResourceTable[i].GenerationTable[j].Value;
                    }
                }
            }
        }

        public Game()
        {
            TestT a = { };

            ResourceTable = new Resource[] {
                new Resource(ResourceTypeId.Undefined, "Undefined", 0,    0,   new ResourceGeneration[]{} ), // must be at index 0 for the program logic
                new Resource(ResourceTypeId.Dollar,    "Dollar",    1,    100, new ResourceGeneration[]{} ), // probably shoud just fix the index to 1 to simplify the code
                new Resource(ResourceTypeId.Corn,      "Corn",      10,   0,   new ResourceGeneration[]{} ),
                new Resource(ResourceTypeId.OilBarrel, "OilBarrel", 50,   0,   new ResourceGeneration[]{} ),
                new Resource(ResourceTypeId.Burger,    "Burger",    100,  0,   new ResourceGeneration[]{} ),
                new Resource(ResourceTypeId.CornField, "CornField", 100,  0,   new ResourceGeneration[]{ new ResourceGeneration(ResourceTypeId.Corn,      1) } ),
                new Resource(ResourceTypeId.OilWell,   "OilWell",   500,  0,   new ResourceGeneration[]{ new ResourceGeneration(ResourceTypeId.OilBarrel, 1) } ),
                new Resource(ResourceTypeId.McDonalds, "McDonalds", 1000, 0,   new ResourceGeneration[]{ new ResourceGeneration(ResourceTypeId.Burger,    1) } ) };
           
        }
    }
}
