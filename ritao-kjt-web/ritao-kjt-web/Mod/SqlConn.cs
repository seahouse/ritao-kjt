using System;
using System.Collections.Generic;
using System.Linq;
using System.Web;

namespace ritao_kjt_web.Mod
{
    public class SqlConn
    {
        public static string getConn()
        {
            return "server=localhost;database=dayamoy;user=sa;pwd=liangyi0328";
            return "server=120.55.165.90,9001;database=dayamoy;user=dayamoyadmin;pwd=dayamoyadmin2015";
        }
    }
}