using System;
using System.Collections.Generic;
using System.Linq;
using System.Web;
using System.Web.UI;
using System.Web.UI.WebControls;
using System.Text;
using Newtonsoft.Json.Linq;
using System.Data;
using System.Data.SqlClient;
using ritao_kjt_web.Mod;

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
            string filename = "1.txt";
            System.IO.File.WriteAllText(filename, str, Encoding.UTF8);

            int index = str.IndexOf("&");
            string[] arr = { };
            if (index != -1)        // 参数大于1个
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
                        string shipTypeID = (string)obj["ShipTypeID"];
                        string trackingNumber = (string)obj["TrackingNumber"];

                        // 写入数据库
                        string strSql = "update 订单 set 发货状态=1, 物流公司='" + shipTypeID + "', 运单号='" + trackingNumber + "', 发票内容='" + commitTime + "' where 订单号='" + merchantOrderID + "'";
                        if (SqlOpt.execSql(strSql) > 0)
                        {
                            Response.Write("SUCCESS");
                            Response.End();
                            return;
                        }
                    }
                }
            }
            else        // 参数少于或等于1项
            {
                int equalindex = str.IndexOf('=');
                if (equalindex != -1)
                {
                    string paramN = str.Substring(0, equalindex);
                    string paramV = str.Substring(equalindex + 1);
                    // ....
                }
            }

            Response.Write("FAILURE");
            Response.End();
        }
    }
}