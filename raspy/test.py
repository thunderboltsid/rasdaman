import ras
con = ras.Connection(username="rasadmin", password="rasadmin")
db = con.database("RASBASE")
txn = db.transaction(rw=True)
q = txn.query("select rgb from rgb")
example = q.eval()
import pdb; pdb.set_trace()
