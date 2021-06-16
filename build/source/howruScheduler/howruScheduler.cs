using System;
using System.IO;
using System.Text;
using System.Net;
using System.Net.Sockets;
using System.Collections.Generic;  
using System.Diagnostics;
using System.Threading;
using System.Threading.Tasks;
using System.Timers;
using System.Globalization;

public class howruScheduler
{
    public static int port;
    public static int initSleep;
    public static string confDir;
    public static string pluginDir;
    public static string dataDir;
    public static string logDir;
    public static string pluginDeclarationsFile;
    public static string strFormat;
    public static string hostName;
    public static bool confDirSet = false;
    public static bool dataDirIsSet = false; 
    public static bool portIsSet = false;
    public static bool sleepIsSet = false;
    public static bool pluginDirIsSet = false;
    public static bool logDirIsSet = false;
    public static bool hasChanges = false;
    public static string[,] declarations;
    public static string[] pluginOutputs;
    public static int[] prevOutputs;
    public static string[] statusChanged;
    public static string[] lastRunTimestamp;
    public static string[] nextRunTimestamp;
    public static string[] lastChangeTimestamp;
    public static int[] pluginRetVals;
    public static StringBuilder logBuilder;

    public static int StrToIntDef(string s, int @default)
    {
       	int number;
        if (int.TryParse(s, out number))
           return number;
        return @default;
    }

    public static void logAppender(string logentry, int level)
    {
       string log;
       string levelinfo;
       string date = DateTime.Now.ToString();
       string newline = "\n";
       switch(level)
       {
          case 0:
             levelinfo = "  INFO\t";
             break;
          case 1:
             levelinfo = "  WARN\t";
             break;
          case 2:
             levelinfo = "  ERROR\t";
             break;
          default:
             levelinfo = "  DEBUG\t";
             break;
       }
       log = date + levelinfo + logentry + newline;
       logBuilder.Append(log);       
    }

    public static void updateLog()
    {
        File.AppendAllText(@logDir, logBuilder.ToString());
        logBuilder.Clear();
    }

    public static int GetConfigurationValues()
    {
      string line;
      StreamReader confFile = new StreamReader(@"/etc/howru/howru.conf");
      while ((line = confFile.ReadLine()) != null)
      {
         var i = line.IndexOf("=");
         if (i > 0) 
         {
            var param = line.Substring(0, i);
            var value = line.Substring(i+1);
            if (param.Equals("scheduler.bindPort")) 
            {
               try
               {
                  port = Int32.Parse(value);
                  portIsSet = true;
               }
               catch (FormatException e)
               {
                  logAppender("Unable to parse '" + value + "'", 4);                  
                  Console.WriteLine($"Unable to parse '{value}'");
                  logAppender(e.Message, 4);
                  Console.WriteLine(e.Message);
                  updateLog();
               }
            }
            if (param.Equals("scheduler.confDir"))
            {
               try
               {
                  if (Directory.Exists(value))
                  {
                     confDir = value;
                     confDirSet = true;
                  }
                  else
                  {
                     DirectoryInfo di = Directory.CreateDirectory(value);
                     logAppender("Configuration directory succesfully created.", 0);
                     Console.WriteLine("The directory was created successfully at {0}.", Directory.GetCreationTime(value));
                     confDirSet = true;
                  }
               }
               catch (Exception e)
               {
                  logAppender("Could not create configuration directory.", 1);
                  Console.WriteLine("The process failed: {0}", e.ToString());
               }
            }
            if (param.Equals("scheduler.format"))
            {
               strFormat = value;
               if (!(strFormat.Equals("json")))
               {
                  logAppender("Format '" + value + "' is not supported.", 1);
                  Console.WriteLine($"Format '{value}' is not supported");
               }
               strFormat = "json";
            }
            if (param.Equals("plugins.initSleepMs"))
            {
               initSleep = StrToIntDef(value, 7000);
               sleepIsSet = true;
               logAppender("Scheduler sleep time is set to " + initSleep + " ms", 0);
            }
            if (param.Equals("scheduler.dataDir"))
            {
               try
               {
                  if (Directory.Exists(value))
                  {
                     dataDir = value;
                     dataDirIsSet = true;
                  }
                  else
                  {
                     DirectoryInfo di = Directory.CreateDirectory(value);
                     logAppender("Data directory was successfully created.", 0);
                     Console.WriteLine("The directory was created successfully at {0}.", Directory.GetCreationTime(value));
                     dataDirIsSet = true;
                  }
               }
               catch (Exception e)
               {
                  logAppender("The process failed. " + e.ToString(), 1);
                  Console.WriteLine("The process failed: {0}", e.ToString());
               }
            }
            if (param.Equals("scheduler.logDir"))
            {
               try
               {
                  if (Directory.Exists(value))
                  {
                     logDir = value + "howru.log";
                     logDirIsSet = true;
                  }
                  else
                  {
                     DirectoryInfo di = Directory.CreateDirectory(value);
                     logAppender("Data directory was successfully created.", 0);
                     Console.WriteLine("The directory was created successfully at {0}.", Directory.GetCreationTime(value));
                     logDir = value + "howru.log";
                     logDirIsSet = true;
                  }
               }
               catch (Exception e)
               {
                  logAppender("The process failed. " + e.ToString(), 1);
                  Console.WriteLine("The process failed: {0}", e.ToString());
               }
            }
            if (param.Equals("plugins.directory"))
            {
               try
               {
                  if (Directory.Exists(value))
                  {
                     pluginDir = value;
                     pluginDirIsSet = true;
                  }
                  else
                  {
                     DirectoryInfo di = Directory.CreateDirectory(value);
                     logAppender("Plugins directory was successfully created.", 0);
                     Console.WriteLine("The directory was created successfully at {0}.", Directory.GetCreationTime(value));
                     pluginDirIsSet = true;
                  }
               }
               catch (Exception e)
               {
                  logAppender("The process failed. " + e.ToString(), 1);
                  Console.WriteLine("The process failed: {0}", e.ToString());
               }
            }
            if (param.Equals("plugins.declaration"))
            {
               if (File.Exists(value))
               {
                  pluginDeclarationsFile = value;
               }
               else
               {
                  logAppender("Plugins declaration file '" + value + "' does not exist.", 2);
                  Console.WriteLine($"ERROR: Plugin declaration file '{value}' does not exist.");
                  return 1;
               }
            }
            logAppender("Parameter " + param + " has value '" + value + "'.", 4);
            Console.WriteLine($"Parameter {param} has value '{value}'");
         }
      }
      confFile.Close();
      if (portIsSet == false) 
      {
         logAppender("Port is not set.", 1);
         Console.WriteLine("Port is not set");
         return 1;
      }
      if (sleepIsSet == false)
      {
         logAppender("Scheduler sleep time is set to 7000 miliseconds.", 0);
         initSleep = 7000;
      }
      updateLog();
      return 0;
    }
  
    public static int countDeclaration(string fileName)
    {
       int i = 0;
       using (StreamReader r = new StreamReader(fileName))
       {
          while (r.ReadLine() != null)
          {
             i++;
          }
       }
       return i -1;
    }

    public static int loadPluginDeclarations()
    {
       bool hasFaults = false;
       string line;
       int counter = 0;
       StreamReader confFile = new StreamReader(pluginDeclarationsFile);
       while ((line = confFile.ReadLine()) != null)
       {
          if (line.IndexOf("#") == -1)
          {
             string[] declaration = line.Split(';');
             if (declaration.Length !=4)
             {
                logAppender("Parameters missing in declaration:", 1);
                Console.WriteLine("WARNING: Parameters missing in declaration:");
                logAppender("Ignoring faulty line: " + line + ".", 1);
                Console.WriteLine($"Ignoring faulty line: {line}");
                hasFaults = true;
             }
             else 
             {
                for (int i = 0; i < 4; i++)
                {
                   declarations[counter,i] = declaration[i];
                }
                counter++;
             }
          }
       }
       updateLog();
       if (hasFaults)
       {
          return 1;
       }
       else
       {
          return 0;
       }
    }

    public static void runPlugin(string pluginToRun, int storeIndex)
    {
       int exitCode = 0;
       String output = "";
       int space = pluginToRun.IndexOf(" ");
       string pluginExe = pluginToRun.Substring(0,space);
       string pluginArgs = pluginToRun.Substring(space+1);
       DateTime startTime = DateTime.Now;
       try 
       {
           Directory.SetCurrentDirectory(@pluginDir);
           Process p = new Process();
           p.StartInfo = new ProcessStartInfo()
           { 
                 WorkingDirectory = pluginDir,
                 FileName = pluginExe,
                 Arguments = pluginArgs,
                 RedirectStandardOutput = true,
                 UseShellExecute = false
           };
           p.Start();
           output = p.StandardOutput.ReadToEnd();
           p.WaitForExit();
           exitCode = p.ExitCode;
        }
        catch (Exception ex)
        {
           logAppender(pluginToRun + " - " + ex.Message, 2);
           Console.WriteLine(ex.Message);
           updateLog();
           return;
        }
	if (prevOutputs[storeIndex] != -1)
	{
		if (exitCode != prevOutputs[storeIndex])
		{
		   statusChanged[storeIndex] = "1";
		}
		else
		{
		   statusChanged[storeIndex] = "0";
		}
		prevOutputs[storeIndex] = exitCode;
	}
        pluginOutputs[storeIndex] = output;
        pluginRetVals[storeIndex] = exitCode;
        DateTime endTime = DateTime.Now;
        TimeSpan elapsedTime = endTime - startTime;
        logAppender(pluginToRun + " executed successfully. Execution took " + (int) elapsedTime.TotalMilliseconds + " ms.", 0);
        updateLog();
    }

    public static string GetHostName()
    {
       String name = "localhost";
       try
       {
          name = Dns.GetHostName();
       }
       catch(SocketException e)
       {
          logAppender("Socket exception caught!", 1);
          logAppender(e.Message, 1);
       } 
       catch (Exception e)
       {
          logAppender("Source: " + e.Source + ", Message: " + e.Message, 2);
       }
       logAppender("Hostname is " + name + ".", 0);
       updateLog();
       return name;
    }

   public static void collectData()
    { 
       int retVal = 0;
       string fileName = @dataDir + "monitor_data.json"; 
       FileStream stream = null;
       DateTime startTime = DateTime.Now;
       try
       {
          stream = new FileStream(fileName, FileMode.Create);
          using (StreamWriter sw = new StreamWriter(stream))
          {
             sw.WriteLine("{");
             sw.WriteLine("   \"host\": {");
             sw.WriteLine("      \"name\":\"" + hostName + "\"");
             sw.WriteLine("   },");
             sw.WriteLine("   \"monitoring\": [");
             for(int i = 0; i < declarations.GetLength(0);i++)
             {  
                try
                {  
                   string pluginStatus = "";
                   string pluginOutput = "";
                   if (Convert.ToInt16(declarations[i,2]) == 1)
                   {  
                      string pluginName = declarations[i,0].Substring(declarations[i,0].IndexOf(" ")+1);
                      retVal = pluginRetVals[i]; 
                      switch(retVal)
                      {  
                         case 0:
                            pluginStatus = "OK";
                            break;
                         case 1:
                            pluginStatus = "WARNING";
                            break;
                         case 2:
                            pluginStatus = "CRITICAL";
                            break;
                         default:
                            pluginStatus = "UNKNOWN";
                            break;
                      }
                      pluginOutput = pluginOutputs[i].Trim();
                      sw.WriteLine("      {");
                      sw.WriteLine("         \"pluginName\":\"" + pluginName + "\",");
                      sw.WriteLine("         \"pluginStatus\":\"" + pluginStatus + "\",");
                      sw.WriteLine("         \"pluginStatusCode\":\"" + retVal.ToString() + "\",");
                      sw.WriteLine("         \"pluginOutput\":\"" + pluginOutput + "\",");
		      sw.WriteLine("         \"pluginStatusChanged\":\"" + statusChanged[i] + "\",");
		      sw.WriteLine("         \"lastChange\":\"" + lastChangeTimestamp[i] + "\",");
                      sw.WriteLine("         \"lastRun\":\"" + lastRunTimestamp[i] + "\",");
                      sw.WriteLine("         \"nextRun\":\"" + nextRunTimestamp[i] + "\"");
                      if (i == declarations.GetLength(0)-1)
                      {
                         sw.WriteLine("      }");
                      }
                      else
                      {
		                 sw.WriteLine("      },");
                      }
                   }
                }
                catch (FormatException e)
                {  
                   logAppender(declarations[i,0] + " has a faulty configuration.", 1);
                   logAppender(e.Message, 4);
                }
             }
             sw.WriteLine(@"  ]");
             sw.WriteLine(@"}"); 
             DateTime endTime = DateTime.Now;
             TimeSpan diff = endTime - startTime;
             logAppender("Data collection took " + (int) diff.TotalMilliseconds + " ms.", 0);
             updateLog();
          }
       }
       finally
       {
          if (stream != null)
             stream.Dispose();
       }
    }


    public static void initScheduler(int numOfP, int msSleep)
    {
       lastRunTimestamp = new string[numOfP+1];
       nextRunTimestamp = new string[numOfP+1];
       lastChangeTimestamp = new string[numOfP+1];
       statusChanged = new string[numOfP+1];
       prevOutputs = new int[numOfP+1]; 
       Console.WriteLine("Initiating scheduler running " + numOfP.ToString() + " plugins");
       logAppender("Initiating scheduler running " + numOfP + " tasks", 0);
       for (int i=0; i < numOfP; i++)
       {
          int active = StrToIntDef(declarations[i,2], 1);
          if (active == 1)
          {
             prevOutputs[i] = -1;
	     statusChanged[i] = "0";
             runPlugin(declarations[i,1], i);
             String timeStamp = DateTime.Now.ToString();
             int nextInterval = StrToIntDef(declarations[i,3], 5);
             String nextRun = DateTime.Now.AddMinutes(nextInterval).ToString();
             lastRunTimestamp[i] = timeStamp;
             lastChangeTimestamp[i] = timeStamp;
             nextRunTimestamp[i] = nextRun;
             logAppender("Next run of " + declarations[i,1] + " is set to " + nextRun, 0);
             Thread.Sleep(msSleep);
          }
          else
          {
             logAppender(declarations[i,0] + " is not active.", 0);
          }   
       }
       nextRunTimestamp[numOfP] = DateTime.Now.AddMinutes(1).ToString();
       logAppender("Next collection time is set to " + nextRunTimestamp[numOfP], 0);
       collectData();
       updateLog();
       //scheduleChecks();
    }

    public static void scheduleChecks()
    {
       System.Timers.Timer scheduler = new System.Timers.Timer();
       scheduler.Elapsed += new ElapsedEventHandler(scheduledRuns);
       scheduler.Interval = 7000;
       scheduler.Start();
    }
 
    public static void runTask(int taskID)
    {
       logAppender(declarations[taskID,1] + " is run in thread with id " + Thread.CurrentThread.ManagedThreadId, 4);
       runPlugin(declarations[taskID,1], taskID);
       String timeStamp = DateTime.Now.ToString();
       int nextInterval = StrToIntDef(declarations[taskID,3], 5);
       String nextRun = DateTime.Now.AddMinutes(nextInterval).ToString();
       lastRunTimestamp[taskID] = timeStamp;
       nextRunTimestamp[taskID] = nextRun;
       if (statusChanged[taskID] == "1")
       {
          lastChangeTimestamp[taskID] = timeStamp;
       }
       logAppender("Next run of " + declarations[taskID,1] + " is set to " + nextRun, 0);  
    }

    public static void scheduledRuns(object source, ElapsedEventArgs e)
    {
       bool hasChanges = false;
       int numOfObjects = nextRunTimestamp.Length;
       for (int i=0; i < numOfObjects-1; i++)
       {
          if (DateTime.Now >= DateTime.Parse(nextRunTimestamp[i], CultureInfo.InvariantCulture))
          {
              int active = StrToIntDef(declarations[i,2], 1);
              if (active == 1)
              {
                 var t = Task.Run(() => runTask(i));
                 t.Wait();
                 hasChanges = true;
                 //collectData();
             }
             else
             {
                Console.WriteLine(declarations[i, 1] + " not active.");
             }
          }  
       }
       if (hasChanges)
       {
          logAppender("Has changes is true. Collect data.", 0);  
          collectData();
          updateLog();
       }
    }

    public static void Main(string[] args)
    {
       logBuilder = new StringBuilder();
       logAppender("Starting howRU scheduler...", 4);
       int retVal = GetConfigurationValues();
       if (retVal == 0)
       {
          logAppender("Configuration read ok.", 0);
          Console.WriteLine("LOG: Configuration read OK");
       }
       else
       {
          logAppender("Config is not valid.", 2);
          Console.WriteLine("ERROR: Config is not valid");
          Environment.Exit(1);
       }
       hostName = GetHostName();
       int decCount = countDeclaration(pluginDeclarationsFile);
       declarations = new string[decCount,4];
       pluginOutputs = new string[decCount];
       pluginRetVals = new int[decCount];
       int pluginDeclarationResult = loadPluginDeclarations();
       if (pluginDeclarationResult != 0)
       {
          logAppender("Problem reading plugin declaration file.", 2);
          Console.WriteLine("ERROR: Problem reading plugin declarations file.");
       }
       else
       {
          logAppender(declarations.GetLength(0).ToString() + " declarations loaded from config.", 0);
          Console.WriteLine("{0} declarations loaded from config.", declarations.GetLength(0));
       }
       updateLog();
       initScheduler(decCount, initSleep);
       logAppender("Starting timer to run checks.", 0);
       updateLog();
       scheduleChecks();
       while ( Console.Read() != 'q' )
       {
;        // we can write anything here if we want, leaving this part blank won’t bother the code execution.
       }
       logAppender("Program got quit signal...quiting!", 4);
       updateLog();
    }
}
