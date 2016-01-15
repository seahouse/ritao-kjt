using System;
using System.Collections.Generic;
using System.Linq;
using System.Web;
using System.Web.UI;
using System.Web.UI.WebControls;
using System.Text;
using Newtonsoft.Json.Linq;

namespace ritao_kjt_web
{
    public partial class OrderSOOutputWarehouse : System.Web.UI.Page
    {
        protected void Page_Load(object sender, EventArgs e)
        {
            System.IO.Stream s = Request.InputStream;
            int count = 0;
            byte[] buffer = new byte[1024];
            StringBuilder builder = new StringBuilder();
            while ((count = s.Read(buffer, 0, 1024)) > 0)
            {
                builder.Append(Encoding.UTF8.GetString(buffer, 0, count));
            }
            s.Flush();
            s.Close();
            s.Dispose();

            string str = builder.ToString();

            int index = str.IndexOf("&");
            string[] arr = { };
            if (index != -1)        // 参数大于等于1个
            {
                arr = str.Split('&');
                for (int i = 0; i < arr.Length; i++)
                {
                    int equalindex = arr[i].IndexOf('=');
                    string paramN = arr[i].Substring(0, equalindex);
                    string paramV = arr[i].Substring(equalindex + 1);

                    if (paramN == "data")
                    {
                        string dataValue = HttpUtility.UrlDecode(paramV);

                        // 解析json
                        JObject obj = JObject.Parse(dataValue);
                        string commitTime = (string)obj["CommitTime"];
                        string merchantOrderID = (string)obj["MerchantOrderID"];
                        int shipTypeID = (int)obj["ShipTypeID"];
                        string trackingNumber = (string)obj["TrackingNumber"];

                        // 写入数据库
                    }
                }
            }
        }
    }
}