import asyncio
import json
import os
import websockets

API_KEY=os.environ['COSYVOICE_API_KEY']
MODEL=os.environ.get('COSYVOICE_MODEL','cosyvoice-v3-plus')
VOICE=os.environ.get('COSYVOICE_VOICE','longhuhu_v3')
TEXT='你好，很高兴见到你！这是一次合成测试。'

async def main():
    headers={'Authorization':f'bearer {API_KEY}','X-DashScope-DataInspection':'enable'}
    async with websockets.connect('wss://dashscope.aliyuncs.com/api-ws/v1/inference',extra_headers=headers,max_size=None) as ws:
        task_id=os.urandom(16).hex()
        await ws.send(json.dumps({
            'header':{'action':'run-task','task_id':task_id,'streaming':'duplex'},
            'payload':{
                'task_group':'audio','task':'tts','function':'SpeechSynthesizer','model':MODEL,
                'parameters':{'text_type':'PlainText','voice':VOICE,'format':'pcm','sample_rate':24000},
                'input':{}
            }
        }))
        async for msg in ws:
            if isinstance(msg,bytes):
                print('binary',len(msg))
                continue
            print('text',msg)
            data=json.loads(msg)
            event=data['header'].get('event')
            if event=='task-started':
                await ws.send(json.dumps({'header':{'action':'continue-task','task_id':task_id,'streaming':'duplex'},'payload':{'input':{'text':TEXT}}}))
                await ws.send(json.dumps({'header':{'action':'finish-task','task_id':task_id,'streaming':'duplex'},'payload':{'input':{}}}))
            if event in ('task-finished','task-failed'):
                break

asyncio.run(main())
