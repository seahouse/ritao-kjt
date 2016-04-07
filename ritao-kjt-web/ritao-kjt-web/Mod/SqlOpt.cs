using System;
using System.Collections.Generic;
using System.Linq;
using System.Web;
using System.Data;
using System.Data.SqlClient;
using ritao_kjt_web.Mod;

namespace ritao_kjt_web.Mod
{
    public class SqlOpt
    {
        public static int execSql(string strSql)
        {
            SqlConnection con = new SqlConnection();
            con.ConnectionString = SqlConn.getConn();

            SqlCommand com = new SqlCommand();
            com.Connection = con;
            com.CommandType = CommandType.Text;
            com.CommandText = strSql;
            con.Open();
            int rowsAffected = com.ExecuteNonQuery();
            con.Close();

            return rowsAffected;
        }
    }
}