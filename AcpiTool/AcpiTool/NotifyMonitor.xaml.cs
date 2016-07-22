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

using AcpiTool;
using AcpiLibrary;

using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.ComponentModel;
using System.Collections.ObjectModel;
using System.Collections.Specialized;
using System.Windows.Threading;
using System.Globalization;
namespace AcpiTool
{
    /// <summary>
    /// Interaction logic for NotifyMonitor.xaml
    /// </summary>
    public partial class NotifyMonitor : Page
    {
        NotifyList notifyList;
        DeviceList deviceList;
        AcpiFilterControl acpiControl;
        IntToIsCheckedConverter intToIsCheckedConverter = new IntToIsCheckedConverter();
        public NotifyMonitor()
        {
            InitializeComponent();
            acpiControl = (App.Current.MainWindow as MainWindow).acpiControl;
            DeviceNode[] deviceNodeArray = null;
            acpiControl.GetDeviceList(ref deviceNodeArray);
            deviceList = new DeviceList(deviceNodeArray, new IsCheckedChangedEventHandler(DeviceItemIsCheckedChanged));
            notifyList = new NotifyList(deviceList);
            //methodListBox.ItemsSource = methodList;
            notifyListBox.ItemsSource = notifyList;
            deviceListBox.ItemsSource = deviceList;


            Binding checkedBinding = new Binding("IsChecked");
            checkedBinding.Source = deviceList;
            checkedBinding.Converter = intToIsCheckedConverter;
            checkBoxSelectAll.SetBinding(CheckBox.IsCheckedProperty, checkedBinding);
        }
        void DeviceItemIsCheckedChanged(Device sender, Object arg)
        {
            acpiControl.setDeviceChecked(sender.Index, sender.IsChecked);
        }
        private void AddNotify(object sender, NotifyEventArgs e)
        {
            notifyList.Add(e);
        }
        private void NotifyMonitorStart(object sender, RoutedEventArgs e)
        {
            bool status;
            buttonStart.IsEnabled = false;
            acpiControl.Notify += new NotifyEventHandler(AddNotify);
            status = acpiControl.StartNotifyMonitor();
            buttonStop.IsEnabled = true;
        }

        private void NotifyMonitorStop(object sender, RoutedEventArgs e)
        {
            buttonStop.IsEnabled = false;
            acpiControl.StopNotifyMonitor();
            acpiControl.Notify -= new NotifyEventHandler(AddNotify);
            buttonStart.IsEnabled = true;
        }
        private void SaveNotifyList(object sender, RoutedEventArgs e)
        {
            SaveFile.SaveNotify(notifyList);
        }
        private void ClearNotifyList(object sender, RoutedEventArgs e)
        {
            notifyList.Clear();
        }

        private void CheckBoxSelectAllClicked(object sender, System.Windows.RoutedEventArgs e)
        {
            foreach (Device device in deviceList)
            {
                if (this.checkBoxSelectAll.IsChecked == true)
                {
                    device.IsChecked = true;
                }
                else if (this.checkBoxSelectAll.IsChecked == false)
                {
                    device.IsChecked = false;
                }
            }
        }

        private void CheckBoxDeviceClicked(object sender, System.Windows.RoutedEventArgs e)
        {
            bool hasCheckedDevice = false, hasUncheckedDevice = false;
            foreach (Device device in deviceList)
            {
                if (device.IsChecked == true)
                {
                    hasCheckedDevice = true;
                }
                else
                {
                    hasUncheckedDevice = true;
                }
                if (hasCheckedDevice && hasUncheckedDevice)
                {
                    break;
                }
            }
            if (hasCheckedDevice && hasUncheckedDevice)
            {
                if (this.checkBoxSelectAll.IsChecked != null)
                {
                    this.checkBoxSelectAll.IsChecked = null;
                }
            }
            else if (hasCheckedDevice)
            {
                if (this.checkBoxSelectAll.IsChecked != true)
                {
                    this.checkBoxSelectAll.IsChecked = true;
                }
            }
            else
            {
                if (this.checkBoxSelectAll.IsChecked != false)
                {
                    this.checkBoxSelectAll.IsChecked = false;
                }
            }
        }
    }
    public class Notify : INotifyPropertyChanged
    {
        public String deviceName;
        public uint data;
        public event PropertyChangedEventHandler PropertyChanged;
        private String notifyInformation;
        public String NotifyInformation
        {
            get
            {
                return notifyInformation;
            }
            set
            {
                notifyInformation = value;
                if (this.PropertyChanged != null)
                {
                    this.PropertyChanged.Invoke(this, new PropertyChangedEventArgs("NotifyInformation"));
                }
            }
        }
        public Notify(String deviceName, uint notifyInformation)
        {
            DateTime dateTime = DateTime.Now;
            this.deviceName = deviceName;
            this.data = notifyInformation;
            this.notifyInformation = deviceName + " notified 0x" + notifyInformation.ToString("X") + "(" +dateTime.TimeOfDay + ")";
        }
    }
    public class NotifyList : ObservableCollection<Notify>, INotifyPropertyChanged
    {
        public event PropertyChangedEventHandler PropertyChanged;
        private DeviceList deviceList;
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
        public NotifyList(DeviceList deviceList)
            : base()
        {
            this.deviceList = deviceList;
            selectedIndex = -1;
        }
        public new void Add(NotifyEventArgs item)
        {
            App.Current.Dispatcher.BeginInvoke(new AddOnDispatchThread(add), item);
            if (selectedIndex == -1)
            {
                SelectedIndex = 0;
            }
        }
        private void add(NotifyEventArgs item)
        {
            //MessageBox.Show(deviceList.Count.ToString());
            //MessageBox.Show(((int)(item.deviceIndex)).ToString());
            (this as ObservableCollection<Notify>).Add(new Notify(deviceList[(int)(item.deviceIndex)].DeviceName, item.notifyInformation));
        }
        private delegate void AddOnDispatchThread(NotifyEventArgs item);

        public new void Clear()
        {
            App.Current.Dispatcher.BeginInvoke(new ClearOnDispatchThread(clear));
            SelectedIndex = -1;
        }
        private void clear()
        {
            (this as ObservableCollection<Notify>).Clear();
        }
        private delegate void ClearOnDispatchThread();
    }
    public delegate void IsCheckedChangedEventHandler(Device sender, Object arg);
    public class Device : INotifyPropertyChanged
    {
        public event PropertyChangedEventHandler PropertyChanged;
        public event IsCheckedChangedEventHandler IsCheckedChanged;
        private uint index;
        public uint Index
        {
            get
            {
                return index;
            }
            set
            {
                index = value;
                if (this.PropertyChanged != null)
                {
                    this.PropertyChanged.Invoke(this, new PropertyChangedEventArgs("Index"));
                }
            }
        }
        private bool isChecked;
        public bool IsChecked
        {
            get
            {
                return isChecked;
            }
            set
            {
                if (isChecked != value)
                {
                    isChecked = value;
                    if (this.PropertyChanged != null)
                    {
                        this.PropertyChanged.Invoke(this, new PropertyChangedEventArgs("IsChecked"));
                    }
                    if (IsCheckedChanged != null)
                    {
                        this.IsCheckedChanged.Invoke(this, null);
                    }
                }
            }
        }
        private String deviceName;
        public String DeviceName
        {
            get
            {
                return deviceName;
            }
            set
            {
                deviceName = value;
                if (this.PropertyChanged != null)
                {
                    this.PropertyChanged.Invoke(this, new PropertyChangedEventArgs("DeviceName"));

                }
            }
        }
        public Device(DeviceNode deviceNode)
        {
            if (deviceNode.devicename != null && deviceNode.devicename.Length != 0)
            {
                this.DeviceName = deviceNode.devicename;
            }
            else
            {
                this.DeviceName = "Unknown Device";
            }
            this.Index = deviceNode.index;
            this.isChecked = false;
        }
    }
    public class DeviceList : ObservableCollection<Device>, INotifyPropertyChanged
    {
        public event PropertyChangedEventHandler PropertyChanged;
        private int isChecked;
        public int IsChecked
        {
            get
            {
                return isChecked;
            }
            set
            {

                if (isChecked != value)
                {
                    isChecked = value;
                    if (this.PropertyChanged != null)
                    {
                        //if (isChecked != null)
                        //{
                        //}
                        this.PropertyChanged.Invoke(this, new PropertyChangedEventArgs("IsChecked"));
                    }
                }
            }
        }
        public DeviceList(DeviceNode[] deviceNodeList, IsCheckedChangedEventHandler isCheckedChangedEventHandler)
            : base()
        {
            if (deviceNodeList == null)
            {
                return;
            }
            foreach (DeviceNode deviceNode in deviceNodeList)
            {
                Device device = new Device(deviceNode);
                device.IsCheckedChanged += isCheckedChangedEventHandler;
                this.Add(device);
            }
        }
        public new void Add(Device item)
        {
            App.Current.Dispatcher.BeginInvoke(new AddOnDispatchThread(add), item);
        }
        private void add(Device item)
        {
            (this as ObservableCollection<Device>).Add(item);
        }
        private delegate void AddOnDispatchThread(Device item);
    }
    public class IntToIsCheckedConverter : IValueConverter
    {
        public object Convert(object value, Type targetType, object parameter, CultureInfo culture)
        {
            if ((int)value == 0)
            {
                return "null";
            }
            else if ((int)value > 0)
            {
                return "true";
            }
            else
            {
                return "false";
            }
        }
        public object ConvertBack(object value, Type targetType, object parameter, CultureInfo culture)
        {
            if ((bool?)value == null)
            {
                return 0;
            }
            else if (((bool?)value).HasValue == false)
            {
                return 0;
            }
            else if (((bool?)value).Value == true)
            {
                return 1;
            }
            else
            {
                return -1;
            }
            //throw new NotImplementedException();
        }

    }
}
