package com.hortahidroponica.controller;

import com.hortahidroponica.model.Horta;
import com.hortahidroponica.model.Parametro;
import jakarta.ws.rs.*;
import jakarta.ws.rs.core.MediaType;

import java.util.List;

@Path("/horta/")
public class HortaController {

    @GET
    @Produces(MediaType.APPLICATION_JSON)
    public List<Horta> getHorta() {
        return Horta.listAll();
    }

    @GET
    @Path("/{id}")
    @Produces(MediaType.APPLICATION_JSON)
    public Horta getHortaById(@PathParam("id") int id) {
        Horta horta = Horta.findById(id);

        if (horta == null) {
            throw new jakarta.ws.rs.NotFoundException("Horta com id " + id + " não encontrada.");
        }

        return horta;
    }

    @GET
    @Path("/{nome}")
    @Produces(MediaType.APPLICATION_JSON)
    public Parametro searchParameterByGardenName(@PathParam("nome") String nome) {
        Horta horta = Horta.find("nome", nome).firstResult();

        if (horta == null) {
            Horta newHorta = new Horta();
            newHorta.nome = nome;
            newHorta.persist();
            throw new BadRequestException("Horta com nome " + nome + " não encontrada. Horta criada com sucesso.");
        }

        if (horta.parametro == null) {
            throw new NotFoundException("Horta com nome " + nome + " não possui parâmetro associado");
        }

        return horta.parametro;
    }

}