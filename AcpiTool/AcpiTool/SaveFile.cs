using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.IO;

using System.Windows.Forms;

namespace AcpiTool
{
    static class SaveFile
    {
        static public String baseDirectory = AppDomain.CurrentDomain.BaseDirectory ;//+ @"Save\";
        static public int SaveNotify(NotifyList notifyList)
        {
            DateTime dateTime = DateTime.Now;
            String path = baseDirectory + "Notify(.txt";// + dateTime.GetDateTimeFormats('s')[0].ToString() +").txt";    OpenFileDialog openFileDialog1 = new OpenFileDialog();

            Stream myStream = null;
            SaveFileDialog saveFileDialog1 = new SaveFileDialog();
            saveFileDialog1.InitialDirectory = baseDirectory;
            saveFileDialog1.Filter = "txt files (*.txt)|*.txt|All files (*.*)|*.*";
            saveFileDialog1.FilterIndex = 1;
            saveFileDialog1.RestoreDirectory = true;

            if (saveFileDialog1.ShowDialog() == DialogResult.OK)
            {
                try
                {
                    if ((myStream = saveFileDialog1.OpenFile()) != null)
                    {
                        using (myStream)
                        {
                            StreamWriter sw = new StreamWriter(myStream);
                            for (int i = 0; i < notifyList.Count; i++)
                            {
                                sw.WriteLine(notifyList[i].NotifyInformation);
                            }
                            sw.Close();
                        }
                    }
                    myStream.Close();
                }
                catch (Exception ex)
                {
                    //MessageBox.Show("Error: Could not read file from disk. Original error: " + ex.Message);
                }
                return 0;
            }
            else
            {
                return -1;
            }
        }
        static public int SaveMethod(MethodList methodList)
        {
            DateTime dateTime = DateTime.Now;

            Stream myStream = null;
            SaveFileDialog saveFileDialog1 = new SaveFileDialog();
            saveFileDialog1.InitialDirectory = baseDirectory;
            saveFileDialog1.Filter = "txt files (*.txt)|*.txt|All files (*.*)|*.*";
            saveFileDialog1.FilterIndex = 1;
            saveFileDialog1.RestoreDirectory = true;

            if (saveFileDialog1.ShowDialog() == DialogResult.OK)
            {
                try
                {
                    if ((myStream = saveFileDialog1.OpenFile()) != null)
                    {
                        using (myStream)
                        {
                            StreamWriter sw = new StreamWriter(myStream);
                            for (int i = 0; i < methodList.Count; i++)
                            {
                                Stream st = new MemoryStream(ASCIIEncoding.Default.GetBytes(methodList[i].MethodInformation));
                                StreamReader sr = new StreamReader(st);
                                sw.WriteLine(methodList[i].MethodPath);
                                while (sr.EndOfStream == false)
                                {
                                    sw.WriteLine("\t" + sr.ReadLine());
                                }
                                sw.Flush();
                                sr.Close();
                                st.Close();
                            }
                            sw.Close();
                        }
                    }
                    myStream.Close();
                }
                catch (Exception ex)
                {
                    //MessageBox.Show("Error: Could not read file from disk. Original error: " + ex.Message);
                }
                return 0;
            }
            else
            {
                return -1;
            }
        }
    }
}
