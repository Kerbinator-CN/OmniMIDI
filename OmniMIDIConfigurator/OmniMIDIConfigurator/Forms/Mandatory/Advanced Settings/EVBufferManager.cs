﻿using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Windows.Forms;

namespace OmniMIDIConfigurator
{
    public partial class EVBufferManager : Form
    {
        ulong installedMemory;

        public EVBufferManager()
        {
            InitializeComponent();
        }

        private void EVBufferManager_Load(object sender, EventArgs e)
        {
            try
            {
                MEMORYSTATUSEX memStatus = new MEMORYSTATUSEX();
                if (!Functions.GlobalMemoryStatusEx(memStatus)) MessageBox.Show("Unknown error.", "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
                installedMemory = memStatus.ullTotalPhys;

                if (Convert.ToInt32(OmniMIDIConfiguratorMain.SynthSettings.GetValue("GetEvBuffSizeFromRAM", "0")) == 1)
                    GetRAMSize.Checked = true;
                else
                    GetRAMSize.Checked = false;

                ArraySize.Maximum = installedMemory;

                PerformCheck();
                PerformRAMCheck();
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.ToString());
            }
        }

        private void PerformCheck()
        {
            if (GetRAMSize.Checked == true)
            {
                OmniMIDIConfiguratorMain.SynthSettings.SetValue("GetEvBuffSizeFromRAM", "1", Microsoft.Win32.RegistryValueKind.DWord);
                decimal evbuffratiotemp = Convert.ToDecimal(OmniMIDIConfiguratorMain.SynthSettings.GetValue("EvBufferMultRatio", "1"));
                ArraySize.Enabled = false;
                RatioVal.Enabled = true;
                ArraySize.Value = installedMemory / 4;
                if (evbuffratiotemp == 1) RatioVal.Value = 128;
                else RatioVal.Value = evbuffratiotemp;
            }
            else
            {
                OmniMIDIConfiguratorMain.SynthSettings.SetValue("GetEvBuffSizeFromRAM", "0", Microsoft.Win32.RegistryValueKind.DWord);
                ulong evbuffsizetemp = Convert.ToUInt64(OmniMIDIConfiguratorMain.SynthSettings.GetValue("EvBufferSize", "4096"));
                ArraySize.Enabled = true;
                RatioVal.Enabled = false;
                if (evbuffsizetemp >= installedMemory) ArraySize.Value = 4096;
                else ArraySize.Value = evbuffsizetemp;
                RatioVal.Value = 1;
            }
        }

        private void PerformRAMCheck()
        {
            ulong check = (ulong)((ArraySize.Value * 4) / RatioVal.Value);

            if (check >= (installedMemory / 8))
            {
                if ((check >= installedMemory) || ((RatioVal.Value == 1) && (GetRAMSize.Checked == true)))
                {
                    WarningSign.Image = OmniMIDIConfigurator.Properties.Resources.wir;
                    WarningLabel.Text = String.Format("ERROR:\nYou cannot use all the available memory!\n(Size: {0} ({1} DWORDs * 4 / {2}))", 
                        SFListFunc.ReturnSoundFontSize(null, "evbuff", (long)ArraySize.Value * 4 / (long)RatioVal.Value), ArraySize.Value, RatioVal.Value);
                    ApplySettings.Enabled = false;
                }
                else
                {
                    WarningSign.Image = OmniMIDIConfigurator.Properties.Resources.wi;
                    WarningLabel.Text = String.Format("WARNING:\nYou should leave at least {0} of RAM available to Windows.\n(Size: {1} ({2} DWORDs * 4 / {3}))",
                        SFListFunc.ReturnSoundFontSize(null, "evbuff", (long)installedMemory / 6),
                        SFListFunc.ReturnSoundFontSize(null, "evbuff", (long)ArraySize.Value * 4 / (long)RatioVal.Value), ArraySize.Value, RatioVal.Value);
                    ApplySettings.Enabled = true;
                }
            }
            else if (check > 1 && check < 128)
            {
                WarningSign.Image = OmniMIDIConfigurator.Properties.Resources.wi;
                WarningLabel.Text = "WARNING: You might experience lag when going below 128 bytes.";
                ApplySettings.Enabled = true;
            }
            else if (check == 1)
            {
                WarningSign.Image = OmniMIDIConfigurator.Properties.Resources.wi;
                WarningLabel.Text = String.Format("The final size will be {0}.\n(({1} DWORDs * 4) / {2})\nGood luck.", SFListFunc.ReturnSoundFontSize(null, "evbuff", (long)ArraySize.Value * 4 / (long)RatioVal.Value), ArraySize.Value, RatioVal.Value);
                ApplySettings.Enabled = true;
            }
            else if (check < 1)
            {
                WarningSign.Image = OmniMIDIConfigurator.Properties.Resources.infoicon;
                WarningLabel.Text = "ERROR:\nHooray, you found the easter egg! Now set a valid value.";
                ApplySettings.Enabled = false;
            }
            else
            {
                WarningSign.Image = OmniMIDIConfigurator.Properties.Resources.successicon;
                WarningLabel.Text = String.Format("The final size will be {0}.\n({1} DWORDs * 4) / {2})\nLooks good!", SFListFunc.ReturnSoundFontSize(null, "evbuff", (long)ArraySize.Value * 4 / (long)RatioVal.Value), ArraySize.Value, RatioVal.Value);
                ApplySettings.Enabled = true;
            }
        }

        private void GetRAMSize_CheckedChanged(object sender, EventArgs e)
        {
            PerformCheck();
            PerformRAMCheck();

            Properties.Settings.Default.Save();
        }

        private void BytesVal_ValueChanged(object sender, EventArgs e)
        {
            PerformRAMCheck();
        }

        private void RatioVal_ValueChanged(object sender, EventArgs e)
        {
            PerformRAMCheck();
        }

        private void ResetSettings_Click(object sender, EventArgs e)
        {
            GetRAMSize.Checked = false;
            ArraySize.Value = 4096;
            RatioVal.Value = 1;
            OmniMIDIConfiguratorMain.SynthSettings.SetValue("GetEvBuffSizeFromRAM", "0", Microsoft.Win32.RegistryValueKind.DWord);
            OmniMIDIConfiguratorMain.SynthSettings.SetValue("EvBufferSize", ArraySize.Value, Microsoft.Win32.RegistryValueKind.QWord);
            OmniMIDIConfiguratorMain.SynthSettings.SetValue("EvBufferMultRatio", RatioVal.Value, Microsoft.Win32.RegistryValueKind.DWord);
        }

        private void ApplySettings_Click(object sender, EventArgs e)
        {
            OmniMIDIConfiguratorMain.SynthSettings.SetValue("EvBufferSize", ArraySize.Value, Microsoft.Win32.RegistryValueKind.QWord);
            OmniMIDIConfiguratorMain.SynthSettings.SetValue("EvBufferMultRatio", RatioVal.Value, Microsoft.Win32.RegistryValueKind.DWord);
            if (Properties.Settings.Default.LiveChanges) OmniMIDIConfiguratorMain.SynthSettings.SetValue("LiveChanges", "1", Microsoft.Win32.RegistryValueKind.DWord);
            Close();
        }

        private void WarningLabel_Click(object sender, EventArgs e)
        {
            MessageBox.Show(String.Format("Leave at least 1/8 of RAM ({0}) available to Windows, to avoid unexpected data loss and system crashes.\n\nRemember, you're responsible of anything that might happen after tampering with this setting.",
                SFListFunc.ReturnSoundFontSize(null, "evbuff", (long)installedMemory / 8)), 
                "What does this warning mean?", MessageBoxButtons.OK, MessageBoxIcon.Warning);
        }
    }

    [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Auto)]
    public class MEMORYSTATUSEX
    {
        public uint dwLength;
        public uint dwMemoryLoad;
        public ulong ullTotalPhys;
        public ulong ullAvailPhys;
        public ulong ullTotalPageFile;
        public ulong ullAvailPageFile;
        public ulong ullTotalVirtual;
        public ulong ullAvailVirtual;
        public ulong ullAvailExtendedVirtual;
        public MEMORYSTATUSEX()
        {
            this.dwLength = (uint)Marshal.SizeOf(typeof(MEMORYSTATUSEX));
        }
    }
}
