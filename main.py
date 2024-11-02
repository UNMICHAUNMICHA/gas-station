from fastapi import FastAPI, Body, HTTPException, Request
from fastapi.responses import HTMLResponse, RedirectResponse
from fastapi.templating import Jinja2Templates
import uvicorn
import qrcode
from io import BytesIO
import base64

app = FastAPI()

templates = Jinja2Templates(directory="templates")

data_store = {}

@app.get("/", response_class=HTMLResponse)
async def read_form(request: Request, message: str = None):
    return templates.TemplateResponse("form.html", {"request": request, "message": message})

@app.post("/submit")
async def submit_data(data: dict = Body(...)):
    fuel_name = data.get('fuel_name')
    unit = data.get('unit')
    liters = data.get('liters')
    amount = data.get('amount')

    if liters < 0 or amount < 0:
        raise HTTPException(status_code=400, detail="Liters and amount must be non-negative.")

    data_store['fuel_name'] = fuel_name
    data_store['unit'] = unit
    data_store['liters'] = liters
    data_store['amount'] = amount

    print(f"Received data: fuel_name={fuel_name}, unit={unit}, liters={liters}, amount={amount}")

    return {"message": "Data received successfully"}




@app.get("/data")
async def get_data():
    data = data_store.copy()
    data_store.clear()
    return data

@app.get("/qr-code", response_class=HTMLResponse)
async def generate_qr_code(request: Request):
    url = "https://webpoo.ncwjjdx.work/"
    
    qr = qrcode.QRCode(version=1, box_size=10, border=5)
    qr.add_data(url)
    qr.make(fit=True)
    
    img = qr.make_image(fill='black', back_color='white')
    
    buf = BytesIO()
    img.save(buf)
    buf.seek(0)
    
    qr_code_b64 = base64.b64encode(buf.getvalue()).decode('utf-8')
    
    return templates.TemplateResponse("qr_code.html", {"request": request, "qr_code": qr_code_b64})

if __name__ == "__main__":
    uvicorn.run(app, host="0.0.0.0", port=8000)
