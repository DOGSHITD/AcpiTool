using System;
using System.Collections;
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
using AcpiLibrary;
namespace AcpiTool
{
    /// <summary>
    /// Interaction logic for CallMethod.xaml
    /// </summary>
    public partial class CallMethod : Page
    {
        MethodTree methodTree;// = new MethodTree();
        MethodToCall methodToCall = new MethodToCall();
        MethodNode[] methodNodeList;
        AcpiFilterControl acpiControl;
        ArgumentList argumentList = new ArgumentList();
        Argument currentArgument = new Argument();
        public CallMethod()
        {
            InitializeComponent();
            acpiControl = (App.Current.MainWindow as MainWindow).acpiControl;
            acpiControl.GetMethodTree(ref methodNodeList);
            methodTree = new MethodTree(methodNodeList, acpiControl.GetRootIndex());
            treeViewMethod.ItemsSource = methodTree;
            comboBoxArgument.ItemsSource = argumentList;
            Binding typeBinding = new Binding("Type");
            typeBinding.Source = currentArgument;
            comboBoxType.SetBinding(ComboBox.SelectedIndexProperty, typeBinding);
            Binding dataBinding = new Binding("Data");
            dataBinding.Source = currentArgument;
            textBoxData.SetBinding(TextBox.TextProperty, dataBinding);
            Binding selectedBinding = new Binding("SelectedIndex");
            selectedBinding.Source = argumentList;
            comboBoxArgument.SetBinding(ListBox.SelectedIndexProperty, selectedBinding);
            //currentArgument.Type = new ARGUMENT_TYPE();
            //currentArgument.Data = "";
        }
        METHOD_ARGUMENT[] outArgument;
        private void ExecuteMethod(object sender, RoutedEventArgs e)
        {
            if (treeViewMethod.SelectedItem != null)
            {
                MethodToCall method = treeViewMethod.SelectedItem as MethodToCall;
                outArgument = null;
                acpiControl.CallMethod((uint)method.Index, method.Name, argumentList.ToMethodArgumentArray(), ref outArgument);
                textBoxOutput.Text = "";
                if (outArgument != null)
                {
                    for (int i = 0; i < outArgument.Length; i++)
                    {
                        Argument tempArgument = new Argument((int)outArgument[i].Type, outArgument[i].Argument, i);
                        textBoxOutput.Text += tempArgument.Information + "\n";
                    }
                }
            }
        }

        private void AddArgument(object sender, RoutedEventArgs e)
        {
            Argument argument = new Argument(currentArgument.Type, currentArgument.Data, argumentList.Count);
            argumentList.Add(argument);
        }

        private void DeleteArgument(object sender, RoutedEventArgs e)
        {
            if (comboBoxArgument.SelectedIndex >= 0 && comboBoxArgument.SelectedIndex < argumentList.Count)
            {
                argumentList.RemoveAt(comboBoxArgument.SelectedIndex);
            }
        }
    }

    public class MethodToCall : INotifyPropertyChanged
    {
        public event PropertyChangedEventHandler PropertyChanged;
        private String name;
        public String Name
        {
            get { return name; }
            set
            {
                name = value;
                if (this.PropertyChanged != null)
                {
                    this.PropertyChanged.Invoke(this, new PropertyChangedEventArgs("Name"));
                }
            }
        }
        private String path;
        public String Path
        {
            get { return path; }
            set
            {
                path = value;
                if (this.PropertyChanged != null)
                {
                    this.PropertyChanged.Invoke(this, new PropertyChangedEventArgs("Path"));
                }
            }
        }
        public int Index;
        private MethodTree children;
        public MethodTree Children
        {
            get { return children; }
            set
            {
                children = value;
                if (this.PropertyChanged != null)
                {
                    this.PropertyChanged.Invoke(this, new PropertyChangedEventArgs("Children"));
                }
            }
        }
        public MethodToCall()
        {
            path = "";
            name = "";
            children = null;

        }
    }
    public class MethodTree : ObservableCollection<MethodToCall>//, INotifyPropertyChanged
    {
        public class MethodComparerClass : IComparer
        {

            // Calls CaseInsensitiveComparer.Compare with the parameters reversed.
            int IComparer.Compare(Object x, Object y)
            {
                return (x as MethodToCall).Name.CompareTo((y as MethodToCall).Name);
            }

        }
        public event PropertyChangedEventHandler PropertyChanged;
        public MethodTree()
            : base()
        {
        }
        public MethodTree(MethodNode[] methodNodeList, int rootIndex)
            : base()
        {
            if (rootIndex == -1)
            {
                return;
            }
            else
            {
                MethodToCall method = new MethodToCall
                {
                    Name = methodNodeList[rootIndex].methodName,
                    Path = methodNodeList[rootIndex].methodName,
                    Index = methodNodeList[rootIndex].methodIndex
                };
                this.Add(method);
                int childIndex = methodNodeList[rootIndex].child;
                if (childIndex != -1)
                {
                    method.Children = new MethodTree(this,methodNodeList,childIndex,method.Path);
                }
            }
        }
        private MethodTree(MethodTree parent, MethodNode[] methodNodeList, int rootIndex, String basePath)
            : base()
        {
            if (rootIndex == -1)
            {
                return;
            }
            else
            {
                int currentIndex = rootIndex;
                MethodToCall method;
                ArrayList childMethodArray = new ArrayList();
                while (currentIndex != -1)
                {
                    if (methodNodeList[currentIndex].methodName.Length != 4)
                    {
                    }
                    method = new MethodToCall
                    {
                        Name = methodNodeList[currentIndex].methodName,
                        Path = basePath + "." + methodNodeList[currentIndex].methodName,
                        Index = methodNodeList[currentIndex].methodIndex
                    };
                    if (method.Name.Length == 4)
                    {

                        childMethodArray.Add(method);
                        childMethodArray.Sort(new MethodComparerClass());
                    }
                    currentIndex = methodNodeList[currentIndex].next;
                    if (currentIndex == rootIndex)
                    {
                        break;
                    }
                }
                for (int i = 0; i < childMethodArray.Count; i ++ )
                {
                    this.Add(childMethodArray[i] as MethodToCall);
                    int childIndex = methodNodeList[(childMethodArray[i] as MethodToCall).Index].child;
                    if (childIndex != -1)
                    {
                        (childMethodArray[i] as MethodToCall).Children = new MethodTree(this, methodNodeList, childIndex, (childMethodArray[i] as MethodToCall).Path);
                    }

                }
            }
        }


        public new void Add(MethodToCall item)
        {
            App.Current.Dispatcher.Invoke(new AddOnDispatchThread(add), item);
        }
        private void add(MethodToCall item)
        {
            (this as ObservableCollection<MethodToCall>).Add(item);
        }
        private delegate void AddOnDispatchThread(MethodToCall item);
    }


    public class Argument : INotifyPropertyChanged
    {
        public event PropertyChangedEventHandler PropertyChanged;
        private ARGUMENT_TYPE type;
        public int Type
        {
            get { return (int)type; }
            set
            {
                type = (ARGUMENT_TYPE)value;
                this.UpdateInformation();
                if (this.PropertyChanged != null)
                {
                    this.PropertyChanged.Invoke(this, new PropertyChangedEventArgs("Type"));
                }
            }
        }
        private String data;
        public String Data
        {
            get { return data; }
            set
            {
                data = value;
                this.UpdateInformation();
                if (this.PropertyChanged != null)
                {
                    this.PropertyChanged.Invoke(this, new PropertyChangedEventArgs("Data"));
                }
            }
        }
        private String information;
        public String Information
        {
            get { return information; }
            set
            {
                information = value;
                if (this.PropertyChanged != null)
                {
                    this.PropertyChanged.Invoke(this, new PropertyChangedEventArgs("Information"));
                }
            }
        }
        private int index;
        public int Index
        {
            get { return index; }
            set
            {
                index = value;
                this.UpdateInformation();
                if (this.PropertyChanged != null)
                {
                    this.PropertyChanged.Invoke(this, new PropertyChangedEventArgs("Index"));
                }
            }
        }
        public Argument()
        {
        }
        public Argument(int type, String data, int index)
        {
            this.Type = type;
            this.Data = data;
            this.Index = index;
            this.Information = "#" + index.ToString() + " Type: " + this.type.ToString() + " Value: " + this.data;
        }
        private void UpdateInformation()
        {
            this.Information = "#" + this.index.ToString() + " Type: " + this.type.ToString() + " Value: " + this.data;
        }
    }
    public class ArgumentList : ObservableCollection<Argument>, INotifyPropertyChanged
    {
        public event PropertyChangedEventHandler PropertyChanged;
        private int selectedIndex;
        public int SelectedIndex
        {
            get { return selectedIndex; }
            set
            {
                selectedIndex = value;
                if (this.PropertyChanged != null)
                {
                    this.PropertyChanged.Invoke(this, new PropertyChangedEventArgs("SelectedIndex"));
                }
            }
        }

        public ArgumentList()
            : base()
        {
            SelectedIndex = -1;
        }

        public new void Add(Argument item)
        {
            App.Current.Dispatcher.Invoke(new AddOnDispatchThread(add), item);
        }
        private void add(Argument item)
        {
            (this as ObservableCollection<Argument>).Add(item);
            SelectedIndex = this.Count - 1;
        }
        private delegate void AddOnDispatchThread(Argument item);

        public new void RemoveAt(int index)
        {
            App.Current.Dispatcher.Invoke(new RemoveAtOnDispatchThread(removeAt), index);
        }
        private void removeAt(int index)
        {
            (this as ObservableCollection<Argument>).RemoveAt(index);
            if (index == Count)
            {
                SelectedIndex = index - 1;
            }
            else
            {
                SelectedIndex = index;
            }
            for (int i = index; i < Count; i++)
            {
                this[i].Index--;
            }
        }
        private delegate void RemoveAtOnDispatchThread(int index);

        public METHOD_ARGUMENT[] ToMethodArgumentArray()
        {
            METHOD_ARGUMENT[] argumentArray = new METHOD_ARGUMENT[this.Count];
            for (int i = 0; i < this.Count; i++ )
            {
                argumentArray[i] = new METHOD_ARGUMENT
                {
                    Type = (ARGUMENT_TYPE)this[i].Type,
                    Argument = this[i].Data
                };
            }
            return argumentArray;
        }
    }
}
