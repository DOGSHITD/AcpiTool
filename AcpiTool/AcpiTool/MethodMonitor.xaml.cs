using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;

using System.ComponentModel;
using System.Collections.ObjectModel;
using System.Collections.Specialized;
using System.Windows.Threading;
using AcpiLibrary;
using System.Collections;
namespace AcpiTool
{
    /// <summary>
    /// Interaction logic for MethodMonitor.xaml
    /// </summary>
    public partial class MethodMonitor : Page
    {
        Method method = new Method();
        private MethodList methodList = new MethodList();
        AcpiFilterControl acpiControl;
        public MethodMonitor()
        {
            InitializeComponent();
            acpiControl = (App.Current.MainWindow as MainWindow).acpiControl;
            Binding selectedBinding = new Binding("SelectedIndex");
            selectedBinding.Source = methodList;
            methodListBox.SetBinding(ListBox.SelectedIndexProperty, selectedBinding);

            methodListBox.ItemsSource = methodList;
        }
        private void AddMethod(object sender, MethodEventArgs e)
        {
            String methodInformation = "";
            IDictionary id = new Dictionary<String, String>(e.methodInformation);
            DateTime dateTime = DateTime.Now;
            foreach (DictionaryEntry de in id)
            {
                methodInformation += de.Key + ": " + de.Value + "\n";
            }
            methodList.Add(new Method(e.methodPath + " (" + dateTime.TimeOfDay + ")", methodInformation));
        }
        private void MethodMonitorStart(object sender, RoutedEventArgs e)
        {
            buttonStart.IsEnabled = false;
            acpiControl.Method += new MethodEventHandler(AddMethod);
            acpiControl.StartMethodMonitor();
            buttonStop.IsEnabled = true;
        }
        private void MethodMonitorStop(object sender, RoutedEventArgs e)
        {
            buttonStop.IsEnabled = false;
            acpiControl.Method -= new MethodEventHandler(AddMethod);
            acpiControl.StopMethodMonitor();
            buttonStart.IsEnabled = true;
        }

        private void SaveMethodList(object sender, RoutedEventArgs e)
        {
            SaveFile.SaveMethod(methodList);
        }
        private void ClearMethodList(object sender, RoutedEventArgs e)
        {
            methodList.Clear();
        }
    }
    public class Method : INotifyPropertyChanged
    {

        private String methodInformation;
        public String MethodInformation
        {
            get
            {
                return methodInformation;
            }
            set
            {
                methodInformation = value;
                if (this.PropertyChanged != null)
                {
                    this.PropertyChanged.Invoke(this, new PropertyChangedEventArgs("MethodInformation"));

                }
            }
        }

        public event PropertyChangedEventHandler PropertyChanged;
        private String methodPath;
        public String MethodPath
        {
            get
            {
                return methodPath;
            }
            set
            {
                methodPath = value;
                if (this.PropertyChanged != null)
                {
                    this.PropertyChanged.Invoke(this, new PropertyChangedEventArgs("MethodPath"));

                }
            }
        }
        public Method()
        {
            this.methodPath = "";
            this.methodInformation = "";
        }
        public Method(String path, String information)
        {
            this.methodPath = path;
            this.methodInformation = information;
        }
    }

    public class MethodList : ObservableCollection<Method>, INotifyPropertyChanged
    {
        public event PropertyChangedEventHandler PropertyChanged;
        private int selectedIndex;
        public int SelectedIndex
        {
            get
            {
                return selectedIndex;
            }
            set
            {
                selectedIndex = value;
                if (this.PropertyChanged != null)
                {
                    this.PropertyChanged.Invoke(this, new PropertyChangedEventArgs("SelectedIndex"));
                }
            }
        }
        public MethodList()
            : base()
        {
            selectedIndex = -1;
        }
        public new void Add(Method item)
        {
            App.Current.Dispatcher.BeginInvoke(new AddOnDispatchThread(add), item);
            if (selectedIndex == -1)
            {
                SelectedIndex = 0;
            }
        }
        private void add(Method item)
        {
            (this as ObservableCollection<Method>).Add(item);
        }
        private delegate void AddOnDispatchThread(Method item);

        public new void Clear()
        {
            App.Current.Dispatcher.BeginInvoke(new ClearOnDispatchThread(clear));
            SelectedIndex = -1;
        }
        private void clear()
        {
            (this as ObservableCollection<Method>).Clear();
        }
        private delegate void ClearOnDispatchThread();
    }
}
