﻿using System;
using System.Collections.Generic;
using System.Text;
using NUnit.Framework;
using InstallerLib;
using System.IO;
using dotNetUnitTestsRunner;
using System.Threading;

namespace dotNetInstallerUnitTests
{
    [TestFixture]
    public class ContinueOnErrorTests
    {
        [Test]
        public void TestContinueOnError()
        {
            ConfigFile configFile = new ConfigFile();
            string markerFilename = Path.Combine(Path.GetTempPath(), Guid.NewGuid().ToString());
            string markerFilename1 = string.Format("{0}.1", markerFilename);
            string markerFilename2 = string.Format("{0}.2", markerFilename);
            SetupConfiguration setupConfiguration = new SetupConfiguration();
            configFile.Children.Add(setupConfiguration);
            ComponentCmd cmd1 = new ComponentCmd();
            setupConfiguration.Children.Add(cmd1);
            cmd1.command = string.Format("cmd.exe /C dir > \"{0}\" & exit /b 1", markerFilename1);
            ComponentCmd cmd2 = new ComponentCmd();
            setupConfiguration.Children.Add(cmd2);
            cmd2.command = string.Format("cmd.exe /C dir > \"{0}\" & exit /b 2", markerFilename2);
            // save config file
            string configFilename = Path.Combine(Path.GetTempPath(), Guid.NewGuid().ToString() + ".xml");
            Console.WriteLine("Writing '{0}'", configFilename);
            configFile.SaveAs(configFilename);
            // execute dotNetInstaller
            Assert.AreEqual(1, dotNetInstallerExeUtils.Run(configFilename));
            Assert.IsTrue(File.Exists(markerFilename1));
            Assert.IsFalse(File.Exists(markerFilename2));
            File.Delete(markerFilename1);
            // allow continue on error (user prompted) -> no effect, this is a prompt that defaults to false in silent mode
            cmd1.allow_continue_on_error = true;
            cmd2.allow_continue_on_error = true;
            configFile.SaveAs(configFilename);
            Assert.AreEqual(1, dotNetInstallerExeUtils.Run(configFilename));
            Assert.IsTrue(File.Exists(markerFilename1));
            Assert.IsFalse(File.Exists(markerFilename2));
            File.Delete(markerFilename1);
            // continue on error by default -> continues on the first and the second component, returns the last error code
            cmd1.default_continue_on_error = true;
            cmd2.default_continue_on_error = true;
            configFile.SaveAs(configFilename);
            // the return code of the first failure is saved
            Assert.AreEqual(1, dotNetInstallerExeUtils.Run(configFilename));
            Assert.IsTrue(File.Exists(markerFilename1));
            Assert.IsTrue(File.Exists(markerFilename2));
            File.Delete(markerFilename1);
            File.Delete(markerFilename2);
            File.Delete(configFilename);
        }
    }
}
