// Copyright (c) Microsoft Corporation
// The Microsoft Corporation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

using System;
using System.Collections.Generic;
using System.IO;
using System.Text.Json.Serialization;

namespace Microsoft.PowerToys.Settings.UI.Library
{
    public class AppSpecificKeysDataModel : KeysDataModel
    {
        private const string KbmRunProgramDelimiter = "<|||>";

        [JsonPropertyName("targetApp")]
        public string TargetApp { get; set; }

        public new List<string> GetMappedOriginalKeys()
        {
            return base.GetMappedOriginalKeys();
        }

        public string TargetAppShortName
        {
            get
            {
                if (!string.IsNullOrEmpty(TargetApp))
                {
                    var parts = TargetApp.Split(KbmRunProgramDelimiter);
                    if (parts.Length >= 1)
                    {
                        return Path.GetFileName(parts[0]);
                    }
                }

                return TargetApp;
            }
        }

        public string TargetAppName
        {
            get
            {
                if (!string.IsNullOrEmpty(TargetApp))
                {
                    var parts = TargetApp.Split(KbmRunProgramDelimiter);
                    if (parts.Length >= 1)
                    {
                        return parts[0];
                    }
                }

                return TargetApp;
            }
        }

        public string TargetAppArgs
        {
            get
            {
                if (!string.IsNullOrEmpty(TargetApp))
                {
                    var parts = TargetApp.Split(KbmRunProgramDelimiter);
                    if (parts.Length >= 2)
                    {
                        return parts[1];
                    }
                }

                return string.Empty;
            }
        }

        public string TargetAppDir
        {
            get
            {
                if (!string.IsNullOrEmpty(TargetApp))
                {
                    var parts = TargetApp.Split(KbmRunProgramDelimiter);
                    if (parts.Length >= 3)
                    {
                        return parts[2];
                    }
                }

                return string.Empty;
            }
        }

        public new List<string> GetMappedNewRemapKeys()
        {
            return base.GetMappedNewRemapKeys();
        }

        public bool Compare(AppSpecificKeysDataModel arg)
        {
            ArgumentNullException.ThrowIfNull(arg);

            // Using Ordinal comparison for internal text
            return OriginalKeys.Equals(arg.OriginalKeys, StringComparison.Ordinal) &&
                NewRemapKeys.Equals(arg.NewRemapKeys, StringComparison.Ordinal) &&
                TargetApp.Equals(arg.TargetApp, StringComparison.Ordinal);
        }
    }
}
