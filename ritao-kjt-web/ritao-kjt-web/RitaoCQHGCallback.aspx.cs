using System;
using System.Collections.Generic;
using System.Linq;
using System.Web;
using System.Web.UI;
using System.Web.UI.WebControls;
using System.Text;
using System.Data;
using System.Data.SqlClient;
using ritao_kjt_web.Mod;

namespace ritao_kjt_web
{
    public partial class RitaoCQHGCallback : System.Web.UI.Page
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

            string strGet = this.Request.QueryString.ToString();
            //string str = Request.Form["data"];
            string method = Request.Form["method"];
            string data = HttpUtility.UrlDecode(Request.Form["data"]);

            // 记录到系统日志
            int len = str.Length > 2000 ? 2000 : str.Length;
            int lenGet = strGet.Length > 2000 ? 2000 : strGet.Length;
            string strSql = "insert into 系统日志(用户名称, 日志类别, 日志日期, 日志内容, 日志备注, 创建日期) " +
                "values ('重庆海关回调', '信息', '" + DateTime.Now.ToString() + "', '" + strGet.Substring(0, lenGet) + "', '" + str.Substring(0, len) + "', '" + DateTime.Now.ToString() + "')";
            SqlOpt.execSql(strSql);

            //int status = 0;
            //if (String.Compare(method, "Order.SOOutputCustoms") == 0)
            //    status = OrderSOOutputCustoms(data);
            //else if (String.Compare(method, "Inventory.ChannelQ4SAdjustRequest") == 0)
            //    status = InventoryChannelQ4SAdjustRequest(data);

            //if (status > 0)
            //{
            //    Response.Write("SUCCESS");
            //    Response.End();
            //    return;
            //}

            //Response.Write("FAILURE");
            //Response.End();
        }
    }
}